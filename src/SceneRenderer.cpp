#include "SceneRenderer.hpp"

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
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "GlfwCreateRendererContext.hpp"
#include "Mesh.hpp"
#include "Resource.hpp"
#include "SceneTree.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "srMesh.hh"
#include "srShader.hpp"
#include "traits/traits.hpp"
#include "vk_utils/Logger.hpp"
#include "vk_utils/vkBuffer.hh"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.h"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkPipeline.hh"
#include "vk_utils/vkSwapChain.h"

namespace gbg {

SceneRenderer::SceneRenderer(RendererContext context)
    : meshes(10),
      materials(10),
      shaders(10),
      instance(context.instance),
      surface(context.surface),
      device(context.device) {
    width = static_cast<uint32_t>(context.width);
    height = static_cast<uint32_t>(context.height);
    initVulkan();
}

void SceneRenderer::setScene(std::shared_ptr<gbg::Scene> scene) {
    this->scene = scene;
    vkDeviceWaitIdle(device.ldevice);
    initResources();
}

void SceneRenderer::setActiveCamera(SceneTreeHandle activeCameraNode) {
    this->activeCameraNode = activeCameraNode;
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
    createGlobalDescriptorSets();

    // Per Material pool and sets
    createMaterialDescriptorPool();  // TODO: crear descriptor pool i

    // Per Model pool and sets
    createModelDescriptorPool();

    // Material DSLs created
    // Material UBO and Textures created also
    processScene();

    // createTexturesImageViews();
    // createTextureSampler();

    createCommandBuffer();
    createSyncObjects();
}

void SceneRenderer::addMesh(Mesh& mesh) {
    LOG("Adding mesh!")

    srMeshHandle vkmh = meshes.create("srMesh::" + mesh.getName());
    srMesh& vkmesh = meshes.get(vkmh);

    for (auto& attr : mesh.getAttributes()) {
        srAttribute attrib = std::visit<srAttribute>(
            [&](auto&& arg) -> srAttribute {
                return srAttribute(device, attr.first, arg.size(),
                                   (AttributeTypes)attr.second.index(),
                                   (void*)arg.data());
            },
            attr.second);

        vkmesh.vertexAttributes.push_back(std::move(attrib));
        vkmesh.indexBuffer = gbg::createIndexBuffer(device, mesh.getFaces());
    }
}

void SceneRenderer::addShader(Shader& shader) {
    srShaderHandle shh = shaders.create("srShader::" + shader.getName());
    srShader& sr_sh = shaders.get(shh);
    VkDescriptorSetLayoutBinding matParmsLayoutBinding{};
    matParmsLayoutBinding.binding = 0;
    matParmsLayoutBinding.descriptorCount = 1;
    matParmsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matParmsLayoutBinding.pImmutableSamplers = nullptr;
    matParmsLayoutBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> materialBindings = {
        matParmsLayoutBinding};

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount =
        static_cast<uint32_t>(materialBindings.size());
    materialLayoutInfo.pBindings = materialBindings.data();

    if (vkCreateDescriptorSetLayout(device.ldevice, &materialLayoutInfo,
                                    nullptr, &sr_sh.layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    std::vector<VkDescriptorSetLayout> desc_sets_layouts = {
        globalDescriptorSetLayout, sr_sh.layout};

    LOG("Adding input bindings...")

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    // TODO: make them a parameter.
    for (const auto& type : shader.getAttributes()) {
        vkVertexInputDescription desc;
        switch (type.second) {
            case FLOAT_ATTR:
                desc = getVertexFloatInputDescription(type.first);
                break;
            case VEC2_ATTR:
                desc = getVertexVector2InputDescription(type.first);
                break;
            case VEC3_ATTR:
                desc = getVertexVector3InputDescription(type.first);
                break;
        }
        bindingDescriptions.push_back(desc.binding_desc);
        attributeDescriptions.push_back(desc.attrib_desc);
    }

    // for the model matrix
    VkPushConstantRange mdl_rg{};
    mdl_rg.offset = 0;
    mdl_rg.size = sizeof(PerObjectPushConstant);
    mdl_rg.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkPushConstantRange> push_constants = {mdl_rg};

    sr_sh.pipeline = createGraphicsPipeline(
        device, shader.getVertShaderCode(), shader.getFragShaderCode(),
        desc_sets_layouts, bindingDescriptions, attributeDescriptions,
        push_constants, msaaSamples, renderPass);
}

void SceneRenderer::addMaterial(Material& mat) {
    srMaterialHandle mth = materials.create("srMaterial::" + mat.getName());

    // TODO: easy to leak memory
    srMaterial& srmt = materials.get(mth);

    // we have the data layed out
    srmt.parameter_values = gbg::allocateParameterValues(mat);

    srmt.paramBuffer = gbg::createBuffer(
        device, srmt.parameter_values.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* data;
    vkMapMemory(device.ldevice, srmt.paramBuffer.memory, 0,
                srmt.paramBuffer.size, 0, &data);
    std::memcpy(data, srmt.parameter_values.data, srmt.parameter_values.size);
    vkUnmapMemory(device.ldevice, srmt.paramBuffer.memory);

    // create descriptor sets

    createMaterialDescriptorSet(srmt, mat);
}

void SceneRenderer::processScene() {
    auto& ms_mg = scene->getMeshManager();
    auto& mt_mg = scene->getMaterialManager();
    auto& sh_mg = scene->getShaderManager();

    for (Mesh& mesh : ms_mg) {
        addMesh(mesh);
    }

    for (Shader& sh : sh_mg) {
        addShader(sh);
    }

    for (Material& mat : mt_mg) {
        addMaterial(mat);
    }
}

void SceneRenderer::cleanup() {
    vkDeviceWaitIdle(device.ldevice);
    cleanupSwapChain();

    // global desc set
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device.ldevice, globalBuffers[i].buffer, nullptr);
        vkFreeMemory(device.ldevice, globalBuffers[i].memory, nullptr);
    }

    vkDestroyDescriptorPool(device.ldevice, globalDescriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device.ldevice, globalDescriptorSetLayout,
                                 nullptr);

    for (const auto& shader : shaders) {
        destroySrShader(device, shader);
    }

    for (const auto& material : materials) {
        destroySrMaterial(device, material);
    }

    for (const auto& mesh : meshes) {
        destroyMesh(device, mesh);
    }

    vkDestroyDescriptorPool(device.ldevice, materialDescPool, nullptr);
    vkDestroyDescriptorPool(device.ldevice, modelDescPool, nullptr);
    vkDestroyDescriptorSetLayout(device.ldevice, modelDescriptorSetLayout,
                                 nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device.ldevice, imageAvailableSemaphores[i],
                           nullptr);
        vkDestroyFence(device.ldevice, inFlightFences[i], nullptr);
    }

    for (int i = 0; i < renderFinishedSemaphores.size(); i++) {
        vkDestroySemaphore(device.ldevice, renderFinishedSemaphores[i],
                           nullptr);
    }

    vkDestroyCommandPool(device.ldevice, device.graphicsCmdPool, nullptr);
    vkDestroyCommandPool(device.ldevice, device.transferCmdPool, nullptr);

    vkDestroyRenderPass(device.ldevice, renderPass, nullptr);

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

    if (vkCreateRenderPass(device.ldevice, &passInfo, nullptr, &renderPass) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

void SceneRenderer::createGlobalDescriptorSetLayouts() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> globalBindings = {
        uboLayoutBinding};

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

    VkDescriptorSetLayoutBinding modelLayoutBinding{};
    uboLayoutBinding.binding = 1;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> modelBindings = {
        modelLayoutBinding};

    VkDescriptorSetLayoutCreateInfo modelLayoutInfo{};
    modelLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    modelLayoutInfo.bindingCount = static_cast<uint32_t>(modelBindings.size());
    modelLayoutInfo.pBindings = modelBindings.data();

    if (vkCreateDescriptorSetLayout(device.ldevice, &modelLayoutInfo, nullptr,
                                    &modelDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    /*
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> inmutableBindings = {
        samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo inmutableLayoutInfo{};
    inmutableLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    inmutableLayoutInfo.bindingCount =
        static_cast<uint32_t>(inmutableBindings.size());
    inmutableLayoutInfo.pBindings = inmutableBindings.data();

    if (vkCreateDescriptorSetLayout(device.ldevice, &inmutableLayoutInfo,
                                    nullptr, &inmutableDescriptorSetLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // TODO: Create descriptor layout from material description
    if (textures.empty()) {
        return;
    }
    std::vector<VkDescriptorSetLayoutBinding> materialBindings;

    VkDescriptorSetLayoutBinding textureLayoutBinding{};
    textureLayoutBinding.binding = 0;
    textureLayoutBinding.descriptorCount = 1;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureLayoutBinding.pImmutableSamplers = nullptr;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings.push_back(textureLayoutBinding);

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount =
        static_cast<uint32_t>(materialBindings.size());
    materialLayoutInfo.pBindings = materialBindings.data();

    VkDescriptorSetLayout matLayout;
    if (vkCreateDescriptorSetLayout(device.ldevice, &materialLayoutInfo,
                                    nullptr, &matLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    materialDescSetLayouts.push_back(matLayout);
    */
}

void SceneRenderer::createFrameBuffers() {
    swapChainFramebuffers.resize(swapChain.swapChainImageViews.size());

    for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImage.view.value(), depthImage.view.value(),
            swapChain.swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
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

bool SceneRenderer::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
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

    transitionImageLayout(depthImage.image, depthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void SceneRenderer::transitionImageLayout(VkImage image, VkFormat format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          uint32_t mipLevels) {
    VkCommandBuffer transBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(transBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    endSingleTimeCommands(device, transBuffer, device.transferCmdPool,
                          device.tqueue);
}

void SceneRenderer::copyBufferToImage(VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    VkBufferImageCopy copyInfo{};
    copyInfo.bufferOffset = 0;
    copyInfo.bufferRowLength = 0;
    copyInfo.bufferImageHeight = 0;

    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.baseArrayLayer = 0;
    copyInfo.imageSubresource.layerCount = 1;
    copyInfo.imageSubresource.mipLevel = 0;

    copyInfo.imageExtent = {width, height, 1};
    copyInfo.imageOffset = {0, 0, 0};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

    gbg::endSingleTimeCommands(device, commandBuffer, device.transferCmdPool,
                               device.tqueue);
}

void SceneRenderer::createTexturesImageViews() {
    for (gbg::vkTexture& texture : textures) {
        gbg::addImageView(texture.textureImage, device.ldevice,
                          VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,
                          textures[0].mipLevels);
    }
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
    createInfo.maxLod = static_cast<float>(textures[0].mipLevels);
    createInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(device.ldevice, &createInfo, nullptr,
                        &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void SceneRenderer::createGlobalShaderResources() {
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
}

void SceneRenderer::createGlobalDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes{};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount =
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
    if (vkCreateDescriptorPool(device.ldevice, &poolInfo, nullptr,
                               &materialDescPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SceneRenderer::createModelDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes{};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(max_obj);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    poolInfo.pPoolSizes = descriptorPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(max_obj);

    if (vkCreateDescriptorPool(device.ldevice, &poolInfo, nullptr,
                               &modelDescPool) != VK_SUCCESS) {
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

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = globalBuffers[i].buffer;
        bufferInfo.range = sizeof(UniformBufferObjects);
        bufferInfo.offset = 0;

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
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

        vkUpdateDescriptorSets(device.ldevice,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void SceneRenderer::createMaterialDescriptorSet(srMaterial& srmat,
                                                Material& mat) {
    ShaderHandle shh = mat.getShaderHandle();

    srShader& srsh = shaders.getRelated(shh);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = materialDescPool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &srsh.layout;

    if (vkAllocateDescriptorSets(device.ldevice, &setInfo,
                                 &srmat.descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets");
    }
    /*
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = textureSampler;
    imageInfo.imageView = textures[i].textureImage.view.value();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    */

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = srmat.paramBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = srmat.descriptor_set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = nullptr;
    // Only needed for other types of descriptors
    descriptorWrite.pTexelBufferView = nullptr;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device.ldevice, 1, &descriptorWrite, 0, nullptr);
}

/*
void SceneRenderer::createStaticDescriptorSets() {
    if (textures.empty()) return;
    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = descriptorPool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &inmutableDescriptorSetLayout;

    if (vkAllocateDescriptorSets(device.ldevice, &setInfo,
                                 &inmutableDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets");
    }

    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.sampler = textureSampler;

    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = inmutableDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &samplerInfo;

    vkUpdateDescriptorSets(device.ldevice, 1, &descriptorWrite, 0, nullptr);
}
*/

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
}

void SceneRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                        uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
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

    auto& md_mg = scene->getModelManager();
    auto& msh_mg = scene->getMeshManager();
    auto& mt_mg = scene->getMaterialManager();
    auto& sh_mg = scene->getShaderManager();
    auto& st_mg = scene->getSceneTreeManager();

    glm::mat4 accumulated_transform = glm::mat4(1.0f);

    std::queue<SceneTreeHandle> Q;
    Q.push(scene->root);
    while (not Q.empty()) {
        SceneTreeHandle visited = Q.front();
        Q.pop();

        SceneTreeNode& stn = st_mg.get(visited);

        auto handle = stn.getResourceH();

        accumulated_transform = accumulated_transform * stn.transform;

        std::visit(
            overloads{
                [&](const ModelHandle& mh) {
                    Model& md = md_mg.get(mh);
                    Material& mt = mt_mg.get(md.getMaterial());
                    Shader& sh = sh_mg.get(mt.getShaderHandle());
                    srShader& srsh = shaders.getRelated(
                        (ResourceHandle)mt.getShaderHandle());
                    srMaterial& srmt = materials.getRelated(md.getMaterial());

                    srMesh& mesh = meshes.getRelated(md.getMesh());

                    vkCmdBindPipeline(commandBuffer,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      srsh.pipeline.pipeline);

                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                    std::vector<VkBuffer> vbuffers;
                    std::vector<VkDeviceSize> voffsets;
                    for (const auto& attrb : mesh.vertexAttributes) {
                        vbuffers.push_back(attrb.buffer.buffer);
                        voffsets.push_back(0);
                    }

                    vkCmdBindVertexBuffers(commandBuffer, 0, vbuffers.size(),
                                           vbuffers.data(), voffsets.data());
                    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.buffer,
                                         0, VK_INDEX_TYPE_UINT32);

                    vkCmdBindDescriptorSets(commandBuffer,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            srsh.pipeline.layout, 1, 1,
                                            &srmt.descriptor_set, 0, nullptr);

                    vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        srsh.pipeline.layout, 0, 1,
                        &globalDescriptorSets[currentFrame], 0, nullptr);

                    PerObjectPushConstant pc{};
                    pc.model = accumulated_transform;
                    vkCmdPushConstants(commandBuffer, srsh.pipeline.layout,
                                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                                       sizeof(PerObjectPushConstant), &pc);

                    vkCmdDrawIndexed(commandBuffer, mesh.indexBuffer.size / 4,
                                     1, 0, 0, 0);
                },
                [&](const CameraHandle& empty) {

                },
                [&](const std::monostate& empty) {

                }},
            handle);

        SceneTreeHandle child = stn.childH;
        while (child) {
            Q.push(child);
            child = st_mg.get(child).nextH;
        }
    }

    vkCmdEndRenderPass(commandBuffer);

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

void SceneRenderer::updateVaryingDescriptorSets(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObjects ubo{};
    auto& st_mg = scene->getSceneTreeManager();
    ubo.view = glm::inverse(st_mg.getTransform(activeCameraNode));
    ubo.proj =
        glm::perspective(glm::radians(45.0f),
                         swapChain.swapChainImageExtent.width /
                             (float)swapChain.swapChainImageExtent.height,
                         0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    memcpy(globalBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void SceneRenderer::drawFrame() {
    // esperem que s'hagi acabat de renderitzar l'últim frame concurrent amb
    // el que toca renderitzar (els si els altres no han acabat no importa)
    vkWaitForFences(device.ldevice, 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device.ldevice, swapChain.swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to recreate swapchain image!");
    }

    updateVaryingDescriptorSets(currentFrame);

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

    if (vkQueueSubmit(device.gqueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(device.pqueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        frameBufferResized) {
        frameBufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
}  // namespace gbg
