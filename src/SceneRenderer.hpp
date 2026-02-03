#pragma once

#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// This forces the perspective proj matrix to use a depth from 0 to 1 when
// it transforms the geometry as vulkan likes.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "GLFW/glfw3.h"
#include "Scene.hpp"
#include "SceneTree.hpp"
#include "srMaterial.hpp"
#include "srPool.hpp"
#include "srShader.hpp"
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
    void setScene(std::shared_ptr<gbg::Scene> scene,
                  std::shared_ptr<gbg::SceneTree> st);
    void run();
    SceneRenderer()
        : meshes(10, (uint8_t)ResourceTypes::MESH),
          materials(1, (uint8_t)ResourceTypes::MATERIAL),
          shaders(1, (uint8_t)ResourceTypes::SHADER) {}

   private:
    enum class ResourceTypes {
        MESH = 0,
        MATERIAL,
        SHADER,
        TEXTURE,
    };
    GLFWwindow* window;
    vkInstance instance;
    VkSurfaceKHR surface;
    vkDevice device;
    VkRenderPass renderPass;

    VkDescriptorPool globalDescriptorPool;
    VkDescriptorPool materialDescPool;
    VkDescriptorPool modelDescPool;

    // Descriptors that change in a frame basis but not per-material
    VkDescriptorSetLayout globalDescriptorSetLayout;
    VkDescriptorSetLayout modelDescriptorSetLayout;

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> globalDescriptorSets;

    gbg::vkSwapChain swapChain;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool frameBufferResized = false;

    srPool<gbg::srShader> shaders;
    srPool<gbg::srMaterial> materials;
    srPool<gbg::vkMesh> meshes;
    const uint32_t max_obj = 1000;
    const uint32_t max_mat = 1000;
    const uint32_t max_tex = 1000;

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

    void processScene();

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

    void createRenderPass();

    void createGlobalDescriptorSetLayouts();

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

    void createGlobalShaderResources();

    void createGlobalDescriptorPool();

    void createMaterialDescriptorPool();

    void createModelDescriptorPool();

    void createGlobalDescriptorSets();

    void createMaterialDescriptorSet(srMaterial& srmat, Material& mat);

    void createModelDescriptorSets();

    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void createSyncObjects();

    VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice pdevice);

    void updateVaryingDescriptorSets(uint32_t currentImage);

    void drawFrame();

    void addMesh(Mesh& mesh);
    void addShader(Shader& shader);
    void addMaterial(Material& shader);
};
}  // namespace gbg
