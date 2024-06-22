#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// This forces the perspective proj matrix to use a depth from 0 to 1 when
// it transforms the geometry as vulkan likes.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <stb_image.h>

#include <cstdint>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <optional>
#include <string>
#include <vector>

#include "Camera.h"
#include "Object.h"
#include "Vertex.h"
#include "vk_Buffer.h"

#pragma once

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

#define NDEBUG

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;
    bool isComplete() {
        return graphicsFamily.has_value() and presentFamily.has_value() and
               transferFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanEngine {
   public:
    void addObject(const Object* object);
    void addCamera(const Camera* camera);
    void run();

    ~VulkanEngine();

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
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainImageExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool graphicsCmdPool;
    VkCommandPool transferCmdPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool frameBufferResized = false;

    std::vector<vk_Buffer> vertexBuffers;
    std::vector<vk_Buffer> indexBuffers;

    std::vector<vk_Buffer> uniformBuffers;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    uint32_t mipLevels;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<const Object*> objects;
    std::vector<const Camera*> cameras;
    int currentCamera = -1;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

   private:
    void setupGlfwCallbacks();

    static void framebufferResizeCallback(GLFWwindow* window, int width,
                                          int height);

    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods);

    void initWindow();

    void initVulkan();

    void mainLoop();

    void cleanupSwapChain();

    void recreateSwapChain();

    // creates an instance. the instance is the object that stores the
    // information about the application that needs to be passed to the
    // implementation to "configure it".
    void createInstance();

    bool checkValidationLayerSupport();

    const std::vector<const char*> getRequiredExtensions();

    void setupDebugMessenger();

    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // static method since it is not concrete to the application instance
    //     checks if signature is correct
    //
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData);

    void createSurface();

    void pickPhysicalDevice();

    uint32_t deviceScore(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    // the logical device is the abstract device that will be responsable for
    // reciving commands (like graphic commands). It is an interface with the
    // physical device

    void createLogicalDevice();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& presentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    void createImageViews();
    void createColorResources();
    static std::vector<char> readFile(const std::string& filename);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFrameBuffers();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkFormat findSupportedFormats(const std::vector<VkFormat>& candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void createDepthResources();

    void createTextureImage();

    void generateMipmaps(VkImage image, VkFormat format, int32_t texWidth,
                         int32_t texHeight, uint32_t mipLevels);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                     VkSampleCountFlagBits numSamples, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& memory);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);

    void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                               VkCommandPool commandPool, VkQueue queue);

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);

    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags,
                                uint32_t mipLevels);

    void createTextureImageView();

    void createTextureSampler();

    void createVertexBuffers();

    void createIndexBuffers();

    void createUniformBuffers();

    void createCommandPools();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void createSyncObjects();

    VkSampleCountFlagBits getMaxUsableSampleCount();

    void updateUniformBuffer(uint32_t currentImage);

    void drawFrame();
};
