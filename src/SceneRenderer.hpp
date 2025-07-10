#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// This forces the perspective proj matrix to use a depth from 0 to 1 when
// it transforms the geometry as vulkan likes.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/hash.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "Logger.h"
#include "Vertex.h"
#include "resourceLoader.h"
#include "vkBuffer.h"
#include "vkImage.h"
#include "vkSwapChain.h"
#include "vkTexture.h"
#include "vkVertexDescriptions.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/pony-cartoon/Pony_cartoon.obj";
const std::string TEXTURE_PATH =
    "models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

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

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // we search for the address of a function called "vkCreateDebu..." and
    // we cast its type to PFN_vkCreateDeb... because vkGetinstanceProcAddr
    // returns a void pointer

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
};

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    std::optional<uint32_t> transferFamily;
    bool isComplete() {
        return graphicsFamily.has_value() and presentFamily.has_value() and
               transferFamily.has_value();
    }
};

struct UniformBufferObjects {
    // Vulkan requires us to align the descriptor data. If it is a scalar to N
    // (4 bytes given 32 bit floats or ints) If it is a vec2 to 2N and if it is
    // a vec4 to 4N. The alignas operator does this for us!
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
class SceneRenderer {
   public:
    void init() {
        initWindow();
        initVulkan();
    }

    void run() {
        initResources();
        mainLoop();
        cleanup();
    }

    void addTexture(const gbg::TextureData& res) {
        gbg::vkTexture texture;
        texture.mipLevels = static_cast<uint32_t>(std::floor(
                                std::log2(std::max(res.width, res.height)))) +
                            1;
        VkDeviceSize imageSize = res.width * res.height * 4;

        gbg::vkBuffer stagingBuffer;

        stagingBuffer = gbg::createBuffer(
            device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device, stagingBuffer.memory, 0, imageSize, 0, &data);
        memcpy(data, res.pixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBuffer.memory);

        texture.textureImage = gbg::createImage(
            physicalDevice, device, res.width, res.height, texture.mipLevels,
            VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        transitionImageLayout(
            texture.textureImage.image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            texture.mipLevels);
        copyBufferToImage(stagingBuffer.buffer, texture.textureImage.image,
                          static_cast<uint32_t>(res.width),
                          static_cast<uint32_t>(res.height));

        generateMipmaps(texture.textureImage.image, VK_FORMAT_R8G8B8A8_SRGB,
                        res.width, res.height, texture.mipLevels);

        vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
        vkFreeMemory(device, stagingBuffer.memory, nullptr);

        textures.push_back(texture);
    }

    void addModel(const gbg::Model& model) {
        std::unordered_map<gbg::Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : model.shapes) {
            for (const auto& index : shape.mesh.indices) {
                gbg::Vertex vertex{};
                vertex.pos = {
                    model.attrib.vertices[3 * index.vertex_index + 0],
                    model.attrib.vertices[3 * index.vertex_index + 1],
                    model.attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.texCoord = {

                    model.attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - model.attrib.texcoords[2 * index.texcoord_index + 1],
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] =
                        static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

   private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSetLayout> materialDescSetLayouts;
    VkDescriptorSetLayout inmutableDescriptorSetLayout;
    VkDescriptorSetLayout variableDescriptorSetLayout;
    std::vector<VkDescriptorSet> materialDescSets;
    std::vector<VkDescriptorSet> variableDescriptorSets;
    VkDescriptorSet inmutableDescriptorSet;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    gbg::vkSwapChain swapChain;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool graphicsCmdPool;
    VkCommandPool transferCmdPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool frameBufferResized = false;
    gbg::vkBuffer vertexBuffer;
    gbg::vkBuffer indexBuffer;

    std::vector<gbg::vkBuffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    std::vector<gbg::vkTexture> textures;
    VkSampler textureSampler;

    gbg::vkImage colorImage;

    gbg::vkImage depthImage;

    std::vector<gbg::Vertex> vertices;
    std::vector<uint32_t> indices;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

   private:
    void setupGlfwCallbacks() {
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width,
                                          int height) {
        auto app =
            reinterpret_cast<SceneRenderer*>(glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods) {
        auto app =
            reinterpret_cast<SceneRenderer*>(glfwGetWindowUserPointer(window));
        if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            std::cout << "W key pressed" << std::endl;
        }
    }

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window =
            glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
        setupGlfwCallbacks();
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayouts();
        createGraphicsPipeline();
        createCommandPools();
        createColorResources();
        createDepthResources();
        createFrameBuffers();
    }

    void initResources() {
        createTexturesImageViews();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDynamicDescriptorSets();
        createMaterialDescriptorSets();
        createInmutableDescriptorSets();
        createCommandBuffer();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanup() {
        cleanupSwapChain();

        vkDestroySampler(device, textureSampler, nullptr);

        for (gbg::vkTexture texture : textures) {
            gbg::destoryImage(texture.textureImage, device);
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
            vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, inmutableDescriptorSetLayout,
                                     nullptr);
        vkDestroyDescriptorSetLayout(device, variableDescriptorSetLayout,
                                     nullptr);
        for (VkDescriptorSetLayout descSet : materialDescSetLayouts) {
            vkDestroyDescriptorSetLayout(device, descSet, nullptr);
        }

        vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);
        vkFreeMemory(device, vertexBuffer.memory, nullptr);

        vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
        vkFreeMemory(device, indexBuffer.memory, nullptr);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, graphicsCmdPool, nullptr);
        vkDestroyCommandPool(device, transferCmdPool, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

            actualExtent.width = std::clamp(actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(
                actualExtent.height, capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain() {
        gbg::SwapChainSupportDetails details =
            gbg::querySwapChainSupport(physicalDevice, surface);
        VkExtent2D extent = chooseSwapExtent(details.capabilities);
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        swapChain = gbg::createSwapChain(physicalDevice, device, surface,
                                         extent, indices.graphicsFamily.value(),
                                         indices.presentFamily.value());
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

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
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error(
                "validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        LOG(extensions)

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext =
                (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        // geting the number of available layers
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);

        // getting the list of available layers propertyes to know if
        // all the requested layers are present needed.
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    const std::vector<const char*> getRequiredExtensions() {
        // get the extensions required by glfw
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // pointer to extension list
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // iterate from extension list address until the last address
        std::vector<const char*> extensions(
            glfwExtensions, glfwExtensions + glfwExtensionCount);

        // enable the messenger extension to print the layers logging if
        // validation layers are enabled
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                         &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }

    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        // All types are followed by EXT because they are part of the
        // VK_EXT_DEBUG_UTILS_EXT_NAME extension
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // types of severity that will be handled by the messenger
        // other way:
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT &
        //     !VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // types of messanges "                                "
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        // the callback to call when a mesa
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
    }

    // static method since it is not concrete to the application instance
    //     checks if signature is correct
    //
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage
                  << std::endl;

        return VK_FALSE;
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with vulkan suport!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        std::multimap<uint32_t, VkPhysicalDevice> scoredDevices;
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        for (const auto& device : devices) {
            uint32_t score = deviceScore(device);
            if (score) {
                scoredDevices.emplace(score, device);
            }
        }

        if (scoredDevices.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        physicalDevice = scoredDevices.rbegin()->second;
        msaaSamples = getMaxUsableSampleCount();
    }

    uint32_t deviceScore(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        uint32_t score = 100;
        if (deviceProperties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 10;
        }
        if (deviceFeatures.geometryShader) {
            score += 1;
        }
        QueueFamilyIndices indices = findQueueFamilies(device);

        if (indices.isComplete() and checkDeviceExtensionSupport(device)) {
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

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

    void createLogicalDevice() {
        // setup queue info struct to pass to the logical device creation who
        // will create the queues for us

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        LOG(indices.graphicsFamily.value())
        LOG(indices.presentFamily.value())
        LOG(indices.transferFamily.value())

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(), indices.presentFamily.value(),
            indices.transferFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount =
            static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            deviceCreateInfo.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr,
                           &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0,
                         &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0,
                         &presentQueue);
        vkGetDeviceQueue(device, indices.transferFamily.value(), 0,
                         &transferQueue);
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 queueFamilies.data());
        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                                 &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transferFamily = i;
            }

            if (indices.isComplete()) break;
            i++;
        }
        return indices;
    }

    void cleanupSwapChain() {
        gbg::destoryImage(colorImage, device);
        gbg::destoryImage(depthImage, device);
        gbg::cleanupSwapChain(swapChain, device);
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    void createImageViews() {
        swapChain.swapChainImageViews.resize(swapChain.swapChainImages.size());
        for (size_t i = 0; i < swapChain.swapChainImages.size(); i++) {
            swapChain.swapChainImageViews[i] = gbg::createImageView(
                swapChain.swapChainImages[i], device,
                swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void createColorResources() {
        VkFormat colorFormat = swapChain.swapChainImageFormat;
        colorImage = gbg::createImage(
            physicalDevice, device, swapChain.swapChainImageExtent.width,
            swapChain.swapChainImageExtent.height, 1, msaaSamples, colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        gbg::addImageView(colorImage, device, colorFormat,
                          VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    static std::vector<char> readFile(const std::string& filename) {
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

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
        return shaderModule;
    }

    void createRenderPass() {
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
        colorAttachmentResolve.stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;
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
        colorAttachmentResolveRef.layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
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

        if (vkCreateRenderPass(device, &passInfo, nullptr, &renderPass) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass");
        }
    }

    void createDescriptorSetLayouts() {
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

        if (vkCreateDescriptorSetLayout(device, &variableLayoutInfo, nullptr,
                                        &variableDescriptorSetLayout) !=
            VK_SUCCESS) {
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

        if (vkCreateDescriptorSetLayout(device, &inmutableLayoutInfo, nullptr,
                                        &inmutableDescriptorSetLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        VkDescriptorSetLayoutBinding textureLayoutBinding{};
        textureLayoutBinding.binding = 0;
        textureLayoutBinding.descriptorCount = 1;
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureLayoutBinding.pImmutableSamplers = nullptr;
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 1> materialBindings = {
            textureLayoutBinding};

        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount =
            static_cast<uint32_t>(materialBindings.size());
        materialLayoutInfo.pBindings = materialBindings.data();

        VkDescriptorSetLayout matLayout;
        if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr,
                                        &matLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        materialDescSetLayouts.push_back(matLayout);
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

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

        auto bindingDescription = gbg::getBindingDescription();
        auto attributeDescriptions = gbg::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions =
            attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
        dynamicState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount =
            static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
            inmutableDescriptorSetLayout};

        descriptorSetLayouts.insert(descriptorSetLayouts.end(),
                                    materialDescSetLayouts.begin(),
                                    materialDescSetLayouts.end());

        descriptorSetLayouts.push_back(variableDescriptorSetLayout);

        VkPipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCreateInfo.setLayoutCount =
            static_cast<uint32_t>(descriptorSetLayouts.size());
        layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        layoutCreateInfo.pushConstantRangeCount = 0;
        layoutCreateInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr,
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

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                      nullptr,
                                      &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFrameBuffers() {
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

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                    &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer cpyCmdBuffer = beginSingleTimeCommands(transferCmdPool);

        VkBufferCopy cpyRegion{};
        cpyRegion.srcOffset = 0;
        cpyRegion.dstOffset = 0;
        cpyRegion.size = size;

        vkCmdCopyBuffer(cpyCmdBuffer, srcBuffer, dstBuffer, 1, &cpyRegion);

        endSingleTimeCommands(cpyCmdBuffer, transferCmdPool, transferQueue);
    }

    VkFormat findSupportedFormats(const std::vector<VkFormat>& candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
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

    VkFormat findDepthFormat() {
        return findSupportedFormats(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
             VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
               format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        depthImage = gbg::createImage(
            physicalDevice, device, swapChain.swapChainImageExtent.width,
            swapChain.swapChainImageExtent.height, 1, msaaSamples, depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        gbg::addImageView(depthImage, device, depthFormat,
                          VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        transitionImageLayout(
            depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    }

    void generateMipmaps(VkImage image, VkFormat format, int32_t texWidth,
                         int32_t texHeight, uint32_t mipLevels) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
                                            &formatProperties);
        if (!(formatProperties.optimalTilingFeatures &
              VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error(
                "texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer =
            beginSingleTimeCommands(graphicsCmdPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                                  mipHeight > 1 ? mipHeight / 2 : 1, 1};

            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);
            if (mipWidth > 1) {
                mipWidth /= 2;
            }
            if (mipHeight > 1) {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer, graphicsCmdPool, graphicsQueue);
    }

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;

        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffer!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                               VkCommandPool commandPool, VkQueue queue) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels) {
        VkCommandBuffer transBuffer = beginSingleTimeCommands(transferCmdPool);

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
                barrier.subresourceRange.aspectMask |=
                    VK_IMAGE_ASPECT_STENCIL_BIT;
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
                   newLayout ==
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(transBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        endSingleTimeCommands(transBuffer, transferCmdPool, transferQueue);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height) {
        VkCommandBuffer commandBuffer =
            beginSingleTimeCommands(transferCmdPool);

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
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyInfo);

        endSingleTimeCommands(commandBuffer, transferCmdPool, transferQueue);
    }

    void createTexturesImageViews() {
        for (gbg::vkTexture& texture : textures) {
            gbg::addImageView(texture.textureImage, device,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_ASPECT_COLOR_BIT, textures[0].mipLevels);
        }
    }

    void createTextureSampler() {
        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.minFilter = VK_FILTER_LINEAR;

        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
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

        if (vkCreateSampler(device, &createInfo, nullptr, &textureSampler) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createVertexBuffer() {
        VkDeviceSize size = vertices.size() * sizeof(vertices[0]);

        gbg::vkBuffer stagingBuffer = gbg::createBuffer(
            device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device, stagingBuffer.memory, 0, size, 0, &data);
        memcpy(data, vertices.data(), size);
        vkUnmapMemory(device, stagingBuffer.memory);

        vertexBuffer = gbg::createBuffer(device, physicalDevice, size,
                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        copyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, size);
        vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
        vkFreeMemory(device, stagingBuffer.memory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize size = indices.size() * sizeof(indices[0]);

        gbg::vkBuffer stagingBuffer = gbg::createBuffer(
            device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device, stagingBuffer.memory, 0, size, 0, &data);
        memcpy(data, indices.data(), size);
        vkUnmapMemory(device, stagingBuffer.memory);

        indexBuffer = gbg::createBuffer(
            device, physicalDevice, size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        copyBuffer(stagingBuffer.buffer, indexBuffer.buffer, size);
        vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
        vkFreeMemory(device, stagingBuffer.memory, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObjects);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            uniformBuffers[i] =
                gbg::createBuffer(device, physicalDevice, bufferSize,
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vkMapMemory(device, uniformBuffers[i].memory, 0, bufferSize, 0,
                        &uniformBuffersMapped[i]);
        }
    }

    void createCommandPools() {
        QueueFamilyIndices queueFamilyIndices =
            findQueueFamilies(physicalDevice);

        // This command pool is to allocate reseteable command buffers
        VkCommandPoolCreateInfo graphicsPoolInfo{};
        graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsPoolInfo.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphicsPoolInfo.queueFamilyIndex =
            queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &graphicsPoolInfo, nullptr,
                                &graphicsCmdPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

        // This command pool is to allocate command buffers that will be short
        // lived
        VkCommandPoolCreateInfo transferPoolInfo{};
        transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        transferPoolInfo.queueFamilyIndex =
            queueFamilyIndices.transferFamily.value();

        if (vkCreateCommandPool(device, &transferPoolInfo, nullptr,
                                &transferCmdPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create transfer command pool!");
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes{};
        descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSizes[0].descriptorCount =
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorPoolSizes[1].descriptorCount = 1;
        descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorPoolSizes[2].descriptorCount =
            static_cast<uint32_t>(textures.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount =
            static_cast<uint32_t>(descriptorPoolSizes.size());
        poolInfo.pPoolSizes = descriptorPoolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) +
                           static_cast<uint32_t>(textures.size()) + 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr,
                                   &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDynamicDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                   variableDescriptorSetLayout);

        VkDescriptorSetAllocateInfo setInfo{};
        setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setInfo.descriptorPool = descriptorPool;
        setInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        setInfo.pSetLayouts = layouts.data();

        variableDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &setInfo,
                                     variableDescriptorSets.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor sets");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
            bufferInfo.range = sizeof(UniformBufferObjects);
            bufferInfo.offset = 0;

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = variableDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType =
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            // Only needed for other types of descriptors
            descriptorWrites[0].pImageInfo = nullptr;
            descriptorWrites[0].pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(
                device, static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    void createMaterialDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(textures.size(),
                                                   materialDescSetLayouts[0]);

        VkDescriptorSetAllocateInfo setInfo{};
        setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setInfo.descriptorPool = descriptorPool;
        setInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        setInfo.pSetLayouts = layouts.data();

        materialDescSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &setInfo,
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

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void createInmutableDescriptorSets() {
        VkDescriptorSetAllocateInfo setInfo{};
        setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setInfo.descriptorPool = descriptorPool;
        setInfo.descriptorSetCount = 1;
        setInfo.pSetLayouts = &inmutableDescriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &setInfo,
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

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    void createCommandBuffer() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = graphicsCmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo,
                                     commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffer!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
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

        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width =
            static_cast<float>(swapChain.swapChainImageExtent.width);
        viewport.height =
            static_cast<float>(swapChain.swapChainImageExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChain.swapChainImageExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &inmutableDescriptorSet,
                                0, nullptr);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 1, 1, materialDescSets.data(),
                                0, nullptr);

        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2,
            1, &variableDescriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                  &imageAvailableSemaphores[i]) != VK_SUCCESS or
                vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                  &renderFinishedSemaphores[i]) != VK_SUCCESS or
                vkCreateFence(device, &fenceInfo, nullptr,
                              &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    VkSampleCountFlagBits getMaxUsableSampleCount() {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice,
                                      &physicalDeviceProperties);
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

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(
                         currentTime - startTime)
                         .count();

        UniformBufferObjects ubo{};
        ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
        ubo.model = glm::rotate(ubo.model, glm::radians(90.0f),
                                glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f),
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj =
            glm::perspective(glm::radians(45.0f),
                             swapChain.swapChainImageExtent.width /
                                 (float)swapChain.swapChainImageExtent.height,
                             0.1f, 100.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void drawFrame() {
        // esperem que s'hagi acabat de renderitzar l'últim frame coucurrent amb
        // el que toca renderitzar (els si els altres no han acabat no importa)
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                        UINT64_MAX);

        uint32_t imageIndex;
        VkResult result =
            vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX,
                                  imageAvailableSemaphores[currentFrame],
                                  VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to recreate swapchain image!");
        }

        updateUniformBuffer(currentFrame);

        // Only reset fence if we know that work is going to be submitted
        // Per tal que es pugui fer submit work
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

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

        VkSemaphore signalSemaphores[] = {
            renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
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

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            frameBufferResized) {
            frameBufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};
