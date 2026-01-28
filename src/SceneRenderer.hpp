#pragma once

#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "GLFW/glfw3.h"

// This forces the perspective proj matrix to use a depth from 0 to 1 when
// it transforms the geometry as vulkan likes.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "Scene.hpp"
#include "SceneTree.hpp"
#include "glm/glm.hpp"
#include "srPool.hpp"
#include "vk_utils/Logger.hpp"
#include "vk_utils/vkBuffer.hh"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.h"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkMesh.hh"
#include "vk_utils/vkSwapChain.h"
#include "vk_utils/vkTexture.h"

namespace gbg {
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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
    void init();
    void setScene(std::shared_ptr<gbg::Scene> scene);
    void run();

   private:
    GLFWwindow* window;
    vkInstance instance;
    VkSurfaceKHR surface;
    vkDevice device;
    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;
    // Descriptors diferent for each material
    std::vector<VkDescriptorSetLayout> materialDescSetLayouts;
    // Descriptors for objects that all shaders have but can´t change
    VkDescriptorSetLayout inmutableDescriptorSetLayout;
    // Descriptors that change in a frame basis but not per-material
    VkDescriptorSetLayout globalDescriptorSetLayout;

    std::vector<VkDescriptorSet> materialDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> globalDescriptorSets;
    VkDescriptorSet inmutableDescriptorSet;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    gbg::vkSwapChain swapChain;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool frameBufferResized = false;

    srPool<gbg::vkMesh> meshes;

    std::vector<gbg::vkTexture> textures;
    VkSampler textureSampler;

    std::vector<gbg::vkBuffer> globalBuffers;
    std::vector<void*> globalBuffersMapped;

    gbg::vkImage colorImage;

    gbg::vkImage depthImage;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::shared_ptr<Scene> scene;
    std::shared_ptr<SceneTree> scene_tree;

   private:
    void setupGlfwCallbacks();

    static void framebufferResizeCallback(GLFWwindow* window, int width,
                                          int height);

    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods);

    void initWindow();

    void initVulkan();

    void initResources();

    void addModels();

    void mainLoop();

    void cleanup();

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void createSwapChain();

    void recreateSwapChain();

    // creates an instance. the instance is the object that stores the
    // information about the application that needs to be passed to the
    // implementation to "configure it".
    void createInstance();

    void createSurface();

    VkPhysicalDevice pickPhysicalDevice();

    uint32_t deviceScore(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    // the logical device is the abstract device that will be responsable for
    // reciving commands (like graphic commands). It is an interface with the
    // physical device
    void createLogicalDevice();

    void cleanupSwapChain();

    void createImageViews();

    void createColorResources();

    static std::vector<char> readFile(const std::string& filename);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createRenderPass();

    void createDescriptorSetLayouts();

    void createGraphicsPipeline();

    void createFrameBuffers();

    VkFormat findSupportedFormats(const std::vector<VkFormat>& candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void createDepthResources();

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);

    void createTexturesImageViews();

    void createTextureSampler();

    void createUniformBuffers();

    void createDescriptorPool();

    void createGlobalDescriptorSets();

    void createMaterialDescriptorSets();

    void createInmutableDescriptorSets();

    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void createSyncObjects();

    VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice pdevice);

    void updateUniformBuffer(uint32_t currentImage);

    void drawFrame();

    struct _CreationVisitor {
        SceneRenderer* renderer;
        void operator()(const ModelHandle& h);
        void operator()(const std::monostate& h);
    };
};
}  // namespace gbg
