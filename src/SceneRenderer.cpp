#include "SceneRenderer.hpp"

#include <sys/param.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "InternalSceneData.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Resource.hpp"
#include "SceneTree.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "loadRendererResources.hpp"
#include "loaders/objLoader.hpp"
#include "macros.hpp"
#include "resourcesUpdate.hpp"
#include "shaderReflexion.hpp"
#include "srMaterial.hpp"
#include "srMesh.hh"
#include "srShader.hpp"
#include "srTexture.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyVulkan.hpp"
#include "traits/traits.hpp"
#include "vk_utils/Logger.hpp"
#include "vk_utils/vkBuffer.hh"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.hh"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkPipeline.hh"
#include "vk_utils/vkSwapChain.h"

namespace gbg {

SceneRenderer::SceneRenderer(RendererContext context)
    : instance(context.instance),
      surface(context.surface),
      device(context.device) {
    width = static_cast<uint32_t>(context.width);
    height = static_cast<uint32_t>(context.height);
    internal_scene = std::make_unique<Scene>();
    internal_resources.scene = internal_scene.get();
    initVulkan();
    initImgui();
}

void SceneRenderer::initImgui() {
    ImGui_ImplVulkan_InitInfo info{};
    info.Instance = instance.instance;
    info.PhysicalDevice = device.pdevice;
    info.Device = device.ldevice;
    info.QueueFamily = getGraphicQueueFamilyIndex(device.pdevice).value();
    info.Queue = device.gqueue;
    info.DescriptorPoolSize = 1000;
    info.MinImageCount = gbg::MAX_FRAMES_IN_FLIGHT;
    info.ImageCount = gbg::MAX_FRAMES_IN_FLIGHT;
    info.PipelineInfoMain.RenderPass = renderPasses.at("color").renderPass;
    info.PipelineInfoMain.Subpass = 0;
    info.PipelineInfoMain.MSAASamples = msaaSamples;
    ImGui_ImplVulkan_Init(&info);
}

void SceneRenderer::setScene(Scene* scene) {
    active_scene_data.scene = scene;
    vkDeviceWaitIdle(device.ldevice);
    initResources();
}

void SceneRenderer::initVulkan() {
    msaaSamples = getMaxUsableSampleCount(device.pdevice);
    createSwapChain();
    createImageViews();
    createRenderPass();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}

void SceneRenderer::initResources() {
    // Model DSL, Global DSL (camera and lights)
    createGlobalDescriptorSetLayouts();

    // Camera and light buffers;
    createGlobalShaderResources();
    createGlobalDescriptorPool();
    createTextureSampler();
    std::cout << "Globals created" << std::endl;
    createGlobalDescriptorSets();

    // Per Material pool and sets
    createMaterialDescriptorPool();

    createShadowResources();

    loadRendererResources(device, globalDescriptorSetLayout, internal_resources,
                          renderPasses, materialDescPool, textureSampler);
    
    createCommandBuffer();
    
    createSyncObjects();
}

void SceneRenderer::fillLightBuffer(uint32_t currentImage) {
    auto& st_mg = active_scene_data.scene->getSceneTreeManager();

    std::vector<vkLight> lightTemporalBuffer;

    glm::mat4 accumulated_transform = glm::mat4(1.0f);

    std::queue<std::pair<SceneTreeHandle, glm::mat4>> Q;
    Q.push({active_scene_data.scene->root, glm::mat4(1.f)});
    while (not Q.empty()) {
        SceneTreeHandle visited = Q.front().first;
        accumulated_transform = Q.front().second;
        Q.pop();

        SceneTreeNode& stn = st_mg.get(visited);

        auto handle = stn.getResourceH();

        accumulated_transform = accumulated_transform * stn.getLocalTransform();

        std::visit(
            overloads{[&](const ModelHandle& mh) {},
                      [&](const CameraHandle& empty) {

                      },
                      [&](const std::monostate& empty) {

                      },
                      [&](const LightHandle& lh) {
                          auto& light = active_scene_data.scene->lh_mg.get(lh);
                          vkLight vklight{};
                          vklight.color = light.color;
                          vklight.direction = light.direction;
                          vklight.position =
                              accumulated_transform * glm::vec4(0., 0., 0., 1.);
                          vklight.proj = glm::perspective(glm::radians(45.0f),
                                                          1.0f, 0.1f, 100.0f);
                          vklight.proj[1][1] *= -1;
                          vklight.proj = vklight.proj *
                                         glm::inverse(accumulated_transform);
                          lightTemporalBuffer.push_back(vklight);
                      }},

            handle);

        SceneTreeHandle child = stn.childH;
        while (child) {
            Q.push({child, accumulated_transform});
            child = st_mg.get(child).nextH;
        }
    }

    memcpy(lightsBuffersMapped[currentImage], lightTemporalBuffer.data(),
           lightTemporalBuffer.size() * sizeof(vkLight));
}


void SceneRenderer::cleanup() {
    vkDeviceWaitIdle(device.ldevice);
    cleanupSwapChain();

    for (int i = 0; i < tracyCtx.size(); i++) {
        TracyVkDestroy(tracyCtx[i]);
    }

    // global desc set
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        destroyBuffer(device, globalBuffers[i]);
        destroyBuffer(device, lightsBuffers[i]);
    }

    vkDestroyDescriptorPool(device.ldevice, globalDescriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device.ldevice, globalDescriptorSetLayout,
                                 nullptr);

    vkDestroySampler(device.ldevice, textureSampler, nullptr);

    for (const auto& shader : active_scene_data.srsh_mg) {
        destroySrShader(device, active_scene_data.srsh_mg.get(shader));
    }

    for (const auto& material : active_scene_data.srmat_mg) {
        destroySrMaterial(device, active_scene_data.srmat_mg.get(material));
    }

    for (const auto& texture : active_scene_data.srtx_mg) {
        destroySrTexture(device, active_scene_data.srtx_mg.get(texture));
    }

    for (const auto& mesh : active_scene_data.srmsh_mg) {
        destroyMesh(device, active_scene_data.srmsh_mg.get(mesh));
    }

    vkDestroyDescriptorPool(device.ldevice, materialDescPool, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device.ldevice, imageAvailableSemaphores[i],
                           nullptr);
        vkDestroyFence(device.ldevice, inFlightFences[i], nullptr);
    }

    for (int i = 0; i < renderFinishedSemaphores.size(); i++) {
        vkDestroySemaphore(device.ldevice, renderFinishedSemaphores[i],
                           nullptr);
    }

    ImGui_ImplVulkan_Shutdown();

    vkDestroyCommandPool(device.ldevice, device.graphicsCmdPool, nullptr);
    vkDestroyCommandPool(device.ldevice, device.transferCmdPool, nullptr);

    for (vkRenderPass renderPass : renderPasses | std::views::values) {
        vkDestroyRenderPass(device.ldevice, renderPass.renderPass, nullptr);
    }

    vkDestroyDevice(device.ldevice, nullptr);

    vkDestroySurfaceKHR(instance.instance, surface, nullptr);
}

VkExtent2D SceneRenderer::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) {
    // if the window manager allows to have a bigger fame buffer than the
    // current window size, it points it by setting the currentExtend to
    // uint32_t limit.
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void SceneRenderer::createSwapChain() {
    gbg::SwapChainSupportDetails details =
        gbg::querySwapChainSupport(device.pdevice, surface);
    VkExtent2D extent = chooseSwapExtent(details.capabilities);
    std::optional<uint32_t> gfamily =
        getGraphicQueueFamilyIndex(device.pdevice);
    std::optional<uint32_t> pfamily =
        getPresentQueueFamilyIndex(device.pdevice, surface);

    swapChain = gbg::createSwapChain(device.pdevice, device.ldevice, surface,
                                     extent, gfamily.value(), pfamily.value());
}

void SceneRenderer::resizeSwapchain(uint32_t width, uint32_t height) {
    frameBufferResized = true;
    this->width = width;
    this->height = height;
}

void SceneRenderer::recreateSwapChain() {
    ZoneScoped;
    vkDeviceWaitIdle(device.ldevice);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}

void SceneRenderer::cleanupSwapChain() {
    gbg::destoryImage(colorImage, device.ldevice);
    gbg::destoryImage(depthImage, device.ldevice);
    gbg::cleanupSwapChain(swapChain, device.ldevice);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device.ldevice, framebuffer, nullptr);
    }
}

void SceneRenderer::createImageViews() {
    swapChain.swapChainImageViews.resize(swapChain.swapChainImages.size());
    for (size_t i = 0; i < swapChain.swapChainImages.size(); i++) {
        swapChain.swapChainImageViews[i] = gbg::createImageView(
            swapChain.swapChainImages[i], device.ldevice,
            swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void SceneRenderer::createColorResources() {
    VkFormat colorFormat = swapChain.swapChainImageFormat;
    colorImage = gbg::createImage(
        device.pdevice, device.ldevice, swapChain.swapChainImageExtent.width,
        swapChain.swapChainImageExtent.height, 1, msaaSamples, colorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    gbg::addImageView(colorImage, device.ldevice, colorFormat,
                      VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void SceneRenderer::createRenderPass() {
    // describe how the color attachment will be treated and interpreted
    // (settings) an attachment is a reference to an image view in the
    // swapchain
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // now we describe another attachment to the pipeline that will be used
    // for depth testing
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChain.swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // create a reference to each attachment to be accessed by a subpass
    // that needs it
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // create a subpass that references a color attachment
    // the index of the pColorAttachments array is referenced
    // from the shader: layout(location = 0) out vec4 outColor
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {
        colorAttachment, depthAttachment, colorAttachmentResolve};

    VkRenderPassCreateInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    passInfo.pAttachments = attachments.data();
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    passInfo.dependencyCount = 1;
    passInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.ldevice, &passInfo, nullptr,
                           &renderPasses["color"].renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
    renderPasses["color"].samples = msaaSamples;
}

void SceneRenderer::createShadowResources() {
    // create images
    VkFormat format = findDepthFormat();

    for (auto& shadowImage : shadowImages) {
        shadowImage = createImage(
            device.pdevice, device.ldevice, shadowSize.width, shadowSize.height,
            1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        addImageView(shadowImage, device.ldevice, format,
                     VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    VkAttachmentDescription depthDesc{};
    depthDesc.format = format;
    depthDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    depthDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 0;
    subpassDesc.pDepthStencilAttachment = &depthRef;

    // giving an external dependency is like putting a pipeline barrier
    // https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/

    std::array<VkSubpassDependency, 2> subDep{};
    subDep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subDep[0].dstSubpass = 0;
    subDep[0].srcStageMask =
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // all commands before need to
                                                // have completed the fragemtn
                                                // shader stage
    subDep[0].dstStageMask =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;  // before entering this
                                                     // stage (we will use in
                                                     // fragemnt shader stage)
    subDep[0].srcAccessMask =
        VK_ACCESS_SHADER_READ_BIT;  // we wait until all reads to the depth
                                    // buffer have completed
    subDep[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // before writting to it
    subDep[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;  // welll

    subDep[1].srcSubpass = 0;
    subDep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subDep[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subDep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subDep[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subDep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subDep[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo rpc{};
    rpc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpc.subpassCount = 1;
    rpc.pSubpasses = &subpassDesc;
    rpc.dependencyCount = subDep.size();
    rpc.pDependencies = subDep.data();
    rpc.attachmentCount = 1;
    rpc.pAttachments = &depthDesc;

    if (vkCreateRenderPass(device.ldevice, &rpc, nullptr,
                           &renderPasses["shadow"].renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Shadow Render Pass!");
    }

    renderPasses["shadow"].samples = VK_SAMPLE_COUNT_1_BIT;

    for (size_t i = 0; i < shadowImages.size(); i++) {
        std::array<VkImageView, 1> attachments = {shadowImages[i].view.value()};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPasses["shadow"].renderPass;
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = shadowSize.width;
        framebufferInfo.height = shadowSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.ldevice, &framebufferInfo, nullptr,
                                &shadowFrameBuffer[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    shadowShader_h = internal_resources.scene->sh_mg.create("Shadow Shader");
    auto& shadowShader = internal_resources.scene->sh_mg.get(shadowShader_h);
    setShaderCode(shadowShader, "data/shaders/shadow.vert", ShaderType::VERTEX);
    reflectShader(shadowShader);

    shadowShader.shadow = false;

    shadowMaterial_h =
        internal_resources.scene->mat_mg.create("Shadow Material");
    auto& shadowMaterial =
        internal_resources.scene->mat_mg.get(shadowMaterial_h);

    shadowMaterial.setShader(shadowShader_h, shadowShader);

    shadowMaterial.setParameterValue<ParameterTypes::INT_PARM>(0, 0);

    updateShader(device, shadowShader_h, internal_resources,
                 renderPasses["shadow"], {globalDescriptorSetLayout});

    updateMaterial(device, shadowMaterial_h, internal_resources,
                   materialDescPool, textureSampler);

    // create descriptor layout
    VkDescriptorSetLayoutBinding shadowBind;
    shadowBind.binding = 0;
    shadowBind.descriptorCount = 1;
    shadowBind.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    shadowBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descSetLayInfo{};
    descSetLayInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayInfo.bindingCount = 1;
    descSetLayInfo.pBindings = &shadowBind;

    if (vkCreateDescriptorSetLayout(device.ldevice, &descSetLayInfo, nullptr,
                                    &shadowDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "Shadow descriptor set layout can't be created!");
    }

    std::vector<VkDescriptorSetLayout> layouts(shadowDescriptorSets.size(),
                                               shadowDescriptorSetLayout);

    // create descriptor
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = materialDescPool;
    allocInfo.descriptorSetCount = shadowDescriptorSets.size();
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device.ldevice, &allocInfo,
                                 shadowDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Shadow descriptor set can't be created!");
    }

    for (int i = 0; i < shadowDescriptorSets.size(); ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = shadowImages[i].view.value();
        imageInfo.sampler = textureSampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &imageInfo;
        write.dstSet = shadowDescriptorSets[i];

        vkUpdateDescriptorSets(device.ldevice, 1, &write, 0, nullptr);
    }
}

void SceneRenderer::createGlobalDescriptorSetLayouts() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding lightsLayoutBinding{};
    lightsLayoutBinding.binding = 2;
    lightsLayoutBinding.descriptorCount = 1;
    lightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightsLayoutBinding.pImmutableSamplers = nullptr;
    lightsLayoutBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> globalBindings = {
        uboLayoutBinding, samplerLayoutBinding, lightsLayoutBinding};

    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount =
        static_cast<uint32_t>(globalBindings.size());
    globalLayoutInfo.pBindings = globalBindings.data();

    if (vkCreateDescriptorSetLayout(device.ldevice, &globalLayoutInfo, nullptr,
                                    &globalDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void SceneRenderer::createFrameBuffers() {
    swapChainFramebuffers.resize(swapChain.swapChainImageViews.size());

    for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImage.view.value(), depthImage.view.value(),
            swapChain.swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPasses.at("color").renderPass;
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain.swapChainImageExtent.width;
        framebufferInfo.height = swapChain.swapChainImageExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.ldevice, &framebufferInfo, nullptr,
                                &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

VkFormat SceneRenderer::findSupportedFormats(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device.pdevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat SceneRenderer::findDepthFormat() {
    return findSupportedFormats(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void SceneRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    depthImage = gbg::createImage(
        device.pdevice, device.ldevice, swapChain.swapChainImageExtent.width,
        swapChain.swapChainImageExtent.height, 1, msaaSamples, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    gbg::addImageView(depthImage, device.ldevice, depthFormat,
                      VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    VkCommandBuffer transBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    transitionImageLayout(device, transBuffer, depthImage.image, depthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

    endSingleTimeCommands(device, transBuffer, device.transferCmdPool,
                          device.tqueue);
}

void SceneRenderer::createTextureSampler() {
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;

    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device.pdevice, &properties);
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = static_cast<float>(3);  // TODO: define constant
    createInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(device.ldevice, &createInfo, nullptr,
                        &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void SceneRenderer::createGlobalShaderResources() {
    // camera
    VkDeviceSize bufferSize = sizeof(UniformBufferObjects);

    globalBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    globalBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        globalBuffers[i] = gbg::createBuffer(
            device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(device.ldevice, globalBuffers[i].memory, 0, bufferSize, 0,
                    &globalBuffersMapped[i]);
    }

    // lights
    bufferSize = sizeof(vkLight) * max_light;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        lightsBuffers[i] = gbg::createBuffer(
            device, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(device.ldevice, lightsBuffers[i].memory, 0, bufferSize, 0,
                    &lightsBuffersMapped[i]);
    }
}

void SceneRenderer::createGlobalDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes{};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSizes[2].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    poolInfo.pPoolSizes = descriptorPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device.ldevice, &poolInfo, nullptr,
                               &globalDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SceneRenderer::createMaterialDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = max_mat;

    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(max_tex);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    poolInfo.pPoolSizes = descriptorPoolSizes.data();
    poolInfo.maxSets = max_mat + max_tex;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    if (vkCreateDescriptorPool(device.ldevice, &poolInfo, nullptr,
                               &materialDescPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SceneRenderer::createGlobalDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               globalDescriptorSetLayout);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = globalDescriptorPool;
    setInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    setInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device.ldevice, &setInfo,
                                 globalDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets");
    }

    // Can it be because bouth frames sample sampler?
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = textureSampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.imageView = VK_NULL_HANDLE;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = globalBuffers[i].buffer;
        bufferInfo.range = sizeof(UniformBufferObjects);
        bufferInfo.offset = 0;

        VkDescriptorBufferInfo lightBufferInfo{};
        lightBufferInfo.buffer = lightsBuffers[i].buffer;
        lightBufferInfo.range = lightsBuffers[i].size;
        lightBufferInfo.offset = 0;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = globalDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        // Only needed for other types of descriptors
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = globalDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = nullptr;
        // Only needed for other types of descriptors
        descriptorWrites[1].pImageInfo = &imageInfo;
        descriptorWrites[1].pTexelBufferView = nullptr;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = globalDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &lightBufferInfo;
        // Only needed for other types of descriptors
        descriptorWrites[2].pImageInfo = nullptr;
        descriptorWrites[2].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device.ldevice,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void SceneRenderer::createCommandBuffer() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device.graphicsCmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device.ldevice, &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }

    for (int i = 0; i < commandBuffers.size(); i++) {
        tracyCtx[i] = TracyVkContext(device.pdevice, device.ldevice,
                                     device.gqueue, commandBuffers[i]);
    }
}

void SceneRenderer::recordDrawModel(VkCommandBuffer commandBuffer,
                                    VkViewport viewport, VkRect2D scissor,
                                    glm::mat4 accumulated_transform, Model& md,
                                    InternalSceneData& model_scene_data,
                                    MaterialHandle override_material) {
    MaterialHandle mth = override_material;
    if (not override_material) {
        bindMaterial(commandBuffer, md.getMaterial(), model_scene_data);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        mth = md.getMaterial();
    }
    
    Material& mt = model_scene_data.scene->mat_mg.get(mth);
    srShader& srsh =
        model_scene_data.srsh_mg.getRelated(mt.getShaderHandle());
    PerObjectPushConstant pc{};
    pc.model = accumulated_transform;
    vkCmdPushConstants(commandBuffer, srsh.pipeline.layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(PerObjectPushConstant), &pc);


    srMesh& mesh = model_scene_data.srmsh_mg.getRelated(md.getMesh());

    std::vector<VkBuffer> vbuffers;
    std::vector<VkDeviceSize> voffsets;
    for (const auto& attrb : mesh.vertexAttributes) {
        vbuffers.push_back(attrb.buffer.buffer);
        voffsets.push_back(0);
    }

    vkCmdBindVertexBuffers(commandBuffer, 0, vbuffers.size(), vbuffers.data(),
                           voffsets.data());
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, mesh.indexBuffer.size / 4, 1, 0, 0, 0);
}

void SceneRenderer::recordDrawScene(
    VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor,
    uint32_t imageIndex, SceneTreeHandle root,
    MaterialHandle override = MaterialHandle()) {
    glm::mat4 accumulated_transform = glm::mat4(1.0f);

    std::queue<std::pair<SceneTreeHandle, glm::mat4>> Q;
    Q.push({root, glm::mat4(1.f)});

    if (override) {
        bindMaterial(commandBuffer, shadowMaterial_h, internal_resources);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    }

    // draw axis
    {
        Model& md = internal_resources.scene->md_mg.getByName("Axis");
        recordDrawModel(commandBuffer, viewport, scissor, accumulated_transform,
                        md, internal_resources, override);
    }

    while (not Q.empty()) {
        SceneTreeHandle visited = Q.front().first;
        accumulated_transform = Q.front().second;
        Q.pop();
        SceneTreeNode& stn = active_scene_data.scene->st_mg.get(visited);
        auto handle = stn.getResourceH();
        accumulated_transform = accumulated_transform * stn.getLocalTransform();

        std::visit(
            overloads{[&](const ModelHandle& mh) {
                          TracyVkZone(tracyCtx[currentFrame], commandBuffer,
                                      "DrawModel");
                          auto& md = active_scene_data.scene->md_mg.get(mh);
                          recordDrawModel(commandBuffer, viewport, scissor,
                                          accumulated_transform, md,
                                          active_scene_data, override);
                      },
                      [&](const CameraHandle& empty) {},
                      [&](const std::monostate& empty) {

                      },
                      [&](const LightHandle& empty) {
                          Model& md = internal_resources.scene->md_mg.getByName(
                              "SpotLight");  // light mesh
                          recordDrawModel(commandBuffer, viewport, scissor,
                                          accumulated_transform, md,
                                          internal_resources, override);
                      }},

            handle);

        SceneTreeHandle child = stn.childH;
        while (child) {
            Q.push({child, accumulated_transform});
            child = active_scene_data.scene->st_mg.get(child).nextH;
        }
    }
}

void SceneRenderer::bindMaterial(VkCommandBuffer commandBuffer,
                                 MaterialHandle math, InternalSceneData& data) {
    Material& mt = data.scene->mat_mg.get(math);
    Shader& sh = data.scene->sh_mg.get(mt.getShaderHandle());
    srShader& srsh = data.srsh_mg.getRelated(mt.getShaderHandle());
    srMaterial& srmt = data.srmat_mg.getRelated(math);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      srsh.pipeline.pipeline);

    if (not mt.getValues().empty()) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                srsh.pipeline.layout, 0, 1,
                                &globalDescriptorSets[currentFrame], 0,
                                nullptr);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            srsh.pipeline.layout, 1, 1, &srmt.descriptor_set, 0,
                            nullptr);

    if(sh.shadow) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                srsh.pipeline.layout, 2, 1,
                                &shadowDescriptorSets[currentFrame], 0, nullptr);
    }

}

void SceneRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                        uint32_t imageIndex) {
    ZoneScoped;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording buffer");
    }

    VkRenderPassBeginInfo shadowRenderPassInfo{};
    shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadowRenderPassInfo.renderPass = renderPasses.at("shadow").renderPass;
    shadowRenderPassInfo.framebuffer = shadowFrameBuffer[currentFrame];
    shadowRenderPassInfo.renderArea.offset = {0, 0};
    shadowRenderPassInfo.renderArea.extent = shadowSize;

    VkClearValue shadowClear = {.depthStencil = {1.0f, 0}};

    shadowRenderPassInfo.clearValueCount = 1;
    shadowRenderPassInfo.pClearValues = &shadowClear;

    vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport shadowViewport{};
    shadowViewport.x = 0.0f;
    shadowViewport.y = 0.0f;
    shadowViewport.width = shadowSize.width;
    shadowViewport.height = shadowSize.height;
    shadowViewport.minDepth = 0.0f;
    shadowViewport.maxDepth = 1.0f;

    VkRect2D shadowScisors{};
    shadowScisors.offset = {0, 0};
    shadowScisors.extent = shadowSize;

    recordDrawScene(commandBuffer, shadowViewport, shadowScisors, imageIndex,
                    active_scene_data.scene->root, shadowMaterial_h);

    vkCmdEndRenderPass(commandBuffer);

    // transition detph map
    transitionImageLayout(device, commandBuffer,
                          shadowImages[currentFrame].image, findDepthFormat(),
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPasses.at("color").renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.swapChainImageExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain.swapChainImageExtent.width);
    viewport.height = static_cast<float>(swapChain.swapChainImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain.swapChainImageExtent;

    recordDrawScene(commandBuffer, viewport, scissor, imageIndex,
                    active_scene_data.scene->root);

    // ImGui
    {
        TracyVkZone(tracyCtx[currentFrame], commandBuffer, "Render ImGui");
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

    TracyVkCollect(tracyCtx[currentFrame], commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void SceneRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(swapChain.swapChainImages.size());
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device.ldevice, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS or

            vkCreateFence(device.ldevice, &fenceInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    for (int i = 0; i < renderFinishedSemaphores.size(); i++) {
        if (vkCreateSemaphore(device.ldevice, &semaphoreInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render semaphores!");
        }
    }
}

VkSampleCountFlagBits SceneRenderer::getMaxUsableSampleCount(
    VkPhysicalDevice pdevice) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(pdevice, &physicalDeviceProperties);
    VkSampleCountFlags counts =
        physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;

    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;

    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;

    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;

    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;

    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}

void SceneRenderer::updateGlobalDescriptorSets(uint32_t currentImage) {
    ZoneScoped;
    // cmaera
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObjects ubo{};
    auto& st_mg = active_scene_data.scene->getSceneTreeManager();
    ubo.view = glm::inverse(
        st_mg.getGlobalTransform(active_scene_data.scene->active_camera));
    ubo.proj =
        glm::perspective(glm::radians(45.0f),
                         swapChain.swapChainImageExtent.width /
                             (float)swapChain.swapChainImageExtent.height,
                         0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    ubo.time = time;
    ubo.obs = st_mg.getGlobalTransform(active_scene_data.scene->active_camera) *
              glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    memcpy(globalBuffersMapped[currentImage], &ubo, sizeof(ubo));

    fillLightBuffer(currentImage);
}

void SceneRenderer::drawFrame() {
    ZoneScoped;
    // esperem que s'hagi acabat de renderitzar l'últim frame concurrent amb
    // el que toca renderitzar (els si els altres no han acabat no importa)
    {
        ZoneScopedN("Wait for Image");
        vkWaitForFences(device.ldevice, 1, &inFlightFences[currentFrame],
                        VK_TRUE, UINT64_MAX);
    }

    uint32_t imageIndex;
    {
        ZoneScopedN("Update GPU Resources");
        VkResult result = vkAcquireNextImageKHR(
            device.ldevice, swapChain.swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
            &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            ImGui::EndFrame();
            return;
        } else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to recreate swapchain image!");
        }

        Scene* scene = active_scene_data.scene;

        for (ShaderHandle shh : scene->sh_mg) {
            updateShader(
                device, shh, active_scene_data, renderPasses.at("color"),
                {globalDescriptorSetLayout, shadowDescriptorSetLayout});
        }

        for (TextureHandle txh : scene->tx_mg) {
            updateTexture(device, txh, active_scene_data, textureSampler);
        }

        for (MaterialHandle math : scene->mat_mg) {
            updateMaterial(device, math, active_scene_data, materialDescPool,
                           textureSampler);
        }

        for (MeshHandle mshh : scene->ms_mg) {
            updateMesh(device, mshh, active_scene_data);
        }

        updateGlobalDescriptorSets(currentFrame);
    }

    // Only reset fence if we know that work is going to be submitted
    // Per tal que es pugui fer submit work
    vkResetFences(device.ldevice, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    // esperarem que l'imatge estigui disponible ( imageAvailableSemaphores
    // ) i també definim el semafor que indicarà que ha acabat el
    // renderitzat (per saber quan presentar el frame)
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    {
        ZoneScopedN("Submit");
        if (vkQueueSubmit(device.gqueue, 1, &submitInfo,
                          inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

    {
        ZoneScopedN("Present");
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores =
            signalSemaphores;  // esperem que acabi el render

        VkSwapchainKHR swapChains[] = {swapChain.swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(device.pqueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            frameBufferResized) {
            frameBufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
}  // namespace gbg
