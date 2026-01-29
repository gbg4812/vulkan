#include "SceneRenderer.hpp"

#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "Mesh.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "srModel.hpp"
#include "vk_utils/vkMesh.hh"

#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "GLFW/glfw3.h"
#include "vk_utils/Logger.hpp"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.h"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkSwapChain.h"

namespace gbg {

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void SceneRenderer::init() {
    initWindow();
    initVulkan();
}

void SceneRenderer::setScene(std::shared_ptr<gbg::Scene> scene,
                             std::shared_ptr<SceneTree> st) {
    this->scene = scene;
    this->scene_tree = st;
}

void SceneRenderer::run() {
    initResources();
    mainLoop();
    cleanup();
}

void SceneRenderer::setupGlfwCallbacks() {
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
}

void SceneRenderer::framebufferResizeCallback(GLFWwindow* window, int width,
                                              int height) {
    auto app =
        reinterpret_cast<SceneRenderer*>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}

void SceneRenderer::keyCallback(GLFWwindow* window, int key, int scancode,
                                int action, int mods) {
    auto app =
        reinterpret_cast<SceneRenderer*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        std::cout << "W key pressed" << std::endl;
    }
}

void SceneRenderer::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window =
        glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
    setupGlfwCallbacks();
}

void SceneRenderer::initVulkan() {
    createInstance();
    createSurface();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}

void SceneRenderer::initResources() {
    addModels();
    // createTexturesImageViews();
    // createTextureSampler();
    // createVertexBuffer();
    // createIndexBuffer();
    createDescriptorSetLayouts();
    createGraphicsPipeline();
    createUniformBuffers();
    createDescriptorPool();
    createGlobalDescriptorSets();
    createMaterialDescriptorSets();
    createInmutableDescriptorSets();
    createCommandBuffer();
    createSyncObjects();
}

void SceneRenderer::_CreationVisitor::operator()(const std::monostate& h) {}

void SceneRenderer::_CreationVisitor::operator()(const ModelHandle& h) {
    auto scene = renderer->scene;
    auto scene_tree = renderer->scene_tree;

    auto& md_mg = scene->getModelManager();
    Model& mdl = md_mg.get(h);
    auto& ms_mg = scene->getMeshManager();
    Mesh& mesh = ms_mg.get(mdl.getMesh());

    LOG("Adding mesh!")

    DepDataHandle vkmh = renderer->meshes.alloc();
    vkMesh& vkmesh = renderer->meshes.get(vkmh);

    DepDataHandle vkmd = renderer->models.alloc();
    srModel& srModel = renderer->models.get(vkmd);
    srModel.mesh = vkmh;

    for (auto& attr : mesh.getAttributes()) {
        vkAttribute attrib = std::visit<vkAttribute>(
            [&](auto&& arg) -> vkAttribute {
                return vkAttribute(renderer->device, attr.first, arg.size(),
                                   (AttributeTypes)attr.second.index(),
                                   (void*)arg.data());
            },
            attr.second);

        vkmesh.vertexAttributes.push_back(std::move(attrib));
        vkmesh.indexBuffer =
            gbg::createIndexBuffer(renderer->device, mesh.getFaces());
    }

    scene_tree->setDepDataHandle(vkmh);
}

void SceneRenderer::addModels() {
    auto& md_mg = scene->getModelManager();
    auto& ms_mg = scene->getMeshManager();

    std::queue<SceneTree*> Q;
    Q.push(scene_tree.get());
    while (not Q.empty()) {
        SceneTree* curr = Q.front();
        Q.pop();
        std::visit<void>(_CreationVisitor{this}, curr->getResourceHandle());
        for (auto& chid : curr->getChildren()) {
            Q.push(chid);
        }
    }
}

void SceneRenderer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device.ldevice);
}

void SceneRenderer::cleanup() {
    cleanupSwapChain();

    vkDestroySampler(device.ldevice, textureSampler, nullptr);

    for (gbg::vkTexture texture : textures) {
        gbg::destoryImage(texture.textureImage, device.ldevice);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyBuffer(device.ldevice, globalBuffers[i].buffer, nullptr);
        vkFreeMemory(device.ldevice, globalBuffers[i].memory, nullptr);
    }

    vkDestroyDescriptorPool(device.ldevice, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device.ldevice, inmutableDescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorSetLayout(device.ldevice, globalDescriptorSetLayout,
                                 nullptr);
    for (VkDescriptorSetLayout descSet : materialDescSetLayouts) {
        vkDestroyDescriptorSetLayout(device.ldevice, descSet, nullptr);
    }

    for (const auto& mesh : meshes) {
        destroyMesh(device, mesh);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device.ldevice, imageAvailableSemaphores[i],
                           nullptr);
        vkDestroySemaphore(device.ldevice, renderFinishedSemaphores[i],
                           nullptr);
        vkDestroyFence(device.ldevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device.ldevice, device.graphicsCmdPool, nullptr);
    vkDestroyCommandPool(device.ldevice, device.transferCmdPool, nullptr);

    vkDestroyPipeline(device.ldevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device.ldevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(device.ldevice, renderPass, nullptr);

    vkDestroyDevice(device.ldevice, nullptr);

    vkDestroySurfaceKHR(instance.instance, surface, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
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
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

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

void SceneRenderer::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device.ldevice);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}

// creates an instance. the instance is the object that stores the
// information about the application that needs to be passed to the
// implementation to "configure it".
void SceneRenderer::createInstance() {
    instance = gbg::createInstance(validationLayers, enableValidationLayers);
}

void SceneRenderer::createSurface() {
    if (glfwCreateWindowSurface(instance.instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

VkPhysicalDevice SceneRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with vulkan suport!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    std::multimap<uint32_t, VkPhysicalDevice> scoredDevices;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, devices.data());
    for (const auto& pdevice : devices) {
        uint32_t score = deviceScore(pdevice);
        if (score) {
            scoredDevices.emplace(score, pdevice);
        }
    }

    if (scoredDevices.empty()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    return scoredDevices.rbegin()->second;
}

uint32_t SceneRenderer::deviceScore(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score = 100;
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 10;
    }
    if (deviceFeatures.geometryShader) {
        score += 1;
    }

    if (getDeviceQueueCompatibility(device, surface) and
        checkDeviceExtensionSupport(device)) {
        gbg::SwapChainSupportDetails swapChainDetails =
            gbg::querySwapChainSupport(device, surface);
        if (swapChainDetails.formats.empty() or
            swapChainDetails.presentModes.empty()) {
            score = 0;
        }

    } else {
        score = 0;
    }

    return score;
}

bool SceneRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// the logical device is the abstract device that will be responsable for
// reciving commands (like graphic commands). It is an interface with the
// physical device
void SceneRenderer::createLogicalDevice() {
    VkPhysicalDevice physicalDevice = pickPhysicalDevice();
    device = createDevice(physicalDevice, deviceExtensions,
                          enableValidationLayers, validationLayers, surface);
    msaaSamples = getMaxUsableSampleCount(device.pdevice);
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

std::vector<char> SceneRenderer::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkShaderModule SceneRenderer::createShaderModule(
    const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device.ldevice, &createInfo, nullptr,
                             &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
    return shaderModule;
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

void SceneRenderer::createDescriptorSetLayouts() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> variableBindings = {
        uboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo variableLayoutInfo{};
    variableLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    variableLayoutInfo.bindingCount =
        static_cast<uint32_t>(variableBindings.size());
    variableLayoutInfo.pBindings = variableBindings.data();

    if (vkCreateDescriptorSetLayout(device.ldevice, &variableLayoutInfo,
                                    nullptr,
                                    &globalDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

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
}

void SceneRenderer::createGraphicsPipeline() {
    auto vertShaderCode = readFile("data/shaders/vert.spv");
    auto fragShaderCode = readFile("data/shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    // TODO: make-it from the shader
    LOG("Adding input bindings...")
    for (const auto& mesh : meshes) {
        for (const auto& attr : mesh.vertexAttributes) {
            auto desc = attr.getAttributeDescriptions();
            bindingDescriptions.push_back(desc.first);
            attributeDescriptions.push_back(desc.second);
        }
    }

    vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = 0.2f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    // depthStencil.maxDepthBounds = 1.0f;
    // depthStencil.minDepthBounds = 0.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    // depthStencil.front = {};
    // depthStencil.back = {};

    // we perform alpha blending here
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // pseudo code explaining blendig operation
    // if (blendEnable) {
    //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb)
    //     <colorBlendOp> (dstColorBlendFactor * oldColor.rgb); finalColor.a
    //     = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp>
    //     (dstAlphaBlendFactor * oldColor.a);
    // } else {
    //     finalColor = newColor;
    // }
    //
    // finalColor = finalColor & colorWriteMask;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    const std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
        globalDescriptorSetLayout};

    if (not textures.empty()) {
        descriptorSetLayouts.insert(descriptorSetLayouts.end(),
                                    materialDescSetLayouts.begin(),
                                    materialDescSetLayouts.end());

        descriptorSetLayouts.push_back(inmutableDescriptorSetLayout);
    }

    LOG("Number of descSets: ")
    LOG(descriptorSetLayouts.size())

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount =
        static_cast<uint32_t>(descriptorSetLayouts.size());
    layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device.ldevice, &layoutCreateInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    // pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device.ldevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device.ldevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(device.ldevice, vertShaderModule, nullptr);
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

void SceneRenderer::createUniformBuffers() {
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

void SceneRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes{};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount = 1;

    descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorPoolSizes[2].descriptorCount =
        static_cast<uint32_t>(std::max<int>(textures.size(), 1));

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    poolInfo.pPoolSizes = descriptorPoolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) +
                       static_cast<uint32_t>(textures.size()) + 1;

    if (vkCreateDescriptorPool(device.ldevice, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SceneRenderer::createGlobalDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               globalDescriptorSetLayout);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = descriptorPool;
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

void SceneRenderer::createMaterialDescriptorSets() {
    if (textures.empty()) return;
    std::vector<VkDescriptorSetLayout> layouts(textures.size(),
                                               materialDescSetLayouts[0]);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = descriptorPool;
    setInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    setInfo.pSetLayouts = layouts.data();

    materialDescSets.resize(textures.size());
    if (vkAllocateDescriptorSets(device.ldevice, &setInfo,
                                 materialDescSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets");
    }
    int i = 0;
    for (VkDescriptorSetLayout layout : layouts) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = textureSampler;
        imageInfo.imageView = textures[i].textureImage.view.value();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = materialDescSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        // Only needed for other types of descriptors
        descriptorWrite.pTexelBufferView = nullptr;
        i++;

        vkUpdateDescriptorSets(device.ldevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void SceneRenderer::createInmutableDescriptorSets() {
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
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain.swapChainImageExtent.width);
    viewport.height = static_cast<float>(swapChain.swapChainImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain.swapChainImageExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (const auto& mesh : meshes) {
        std::vector<VkBuffer> vbuffers;
        std::vector<VkDeviceSize> voffsets;
        for (const auto& attrb : mesh.vertexAttributes) {
            vbuffers.push_back(attrb.buffer.buffer);
            voffsets.push_back(0);
        }
        vkCmdBindVertexBuffers(commandBuffer, 0, vbuffers.size(),
                               vbuffers.data(), voffsets.data());
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        if (not textures.empty()) {
            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                2, 1, &inmutableDescriptorSet, 0, nullptr);
            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                1, 1, materialDescSets.data(), 0, nullptr);
        }

        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
            1, &globalDescriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, mesh.indexBuffer.size / 4, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void SceneRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device.ldevice, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS or
            vkCreateSemaphore(device.ldevice, &semaphoreInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS or
            vkCreateFence(device.ldevice, &fenceInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
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

void SceneRenderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObjects ubo{};
    ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    ubo.model = glm::rotate(ubo.model, glm::radians(90.0f),
                            glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view =
        glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj =
        glm::perspective(glm::radians(45.0f),
                         swapChain.swapChainImageExtent.width /
                             (float)swapChain.swapChainImageExtent.height,
                         0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    memcpy(globalBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void SceneRenderer::drawFrame() {
    // esperem que s'hagi acabat de renderitzar l'últim frame coucurrent amb
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

    updateUniformBuffer(currentFrame);

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

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
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
