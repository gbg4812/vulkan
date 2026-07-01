#pragma once

#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "GlfwCreateRendererContext.hpp"
#include "Material.hpp"
#include "SceneTree.hpp"
#include "srMesh.hh"

// This forces the perspective proj matrix to use a depth from 0 to 1 when
// it transforms the geometry as vulkan likes.
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "Scene.hpp"
#include "srLight.hpp"
#include "srMaterial.hpp"
#include "srShader.hpp"
#include "srTexture.hpp"
#include "tracy/TracyVulkan.hpp"
#include "vk_utils/vkBuffer.hh"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.hh"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkSwapChain.h"

namespace gbg {

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct PerObjectPushConstant {
    glm::mat4 model;
};

struct UniformBufferObjects {
    // Vulkan requires us to align the descriptor data. If it is a scalar to N
    // (4 bytes given 32 bit floats or ints) If it is a vec2 to 2N and if it is
    // a vec4 to 4N. The alignas operator does this for us!
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

    // camera pos
    alignas(16) glm::vec3 obs;

    // time
    alignas(16) float time;
};

struct InternalSceneData {
    srMaterialManager srmat_mg;
    srShaderManager srsh_mg;
    srTextureManager srtx_mg;
    srMeshManager srmsh_mg;
    srLightManager srlight_mg;

    Scene* scene;
};

class SceneRenderer {
   public:
    SceneRenderer(RendererContext context);
    void setScene(gbg::Scene* scene);
    void run();
    void resizeSwapchain(uint32_t width, uint32_t height);
    void cleanup();
    void drawFrame();

   private:
    vkInstance instance;
    VkSurfaceKHR surface;
    vkDevice device;
    uint32_t width;
    uint32_t height;

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

    std::array<TracyVkCtx, MAX_FRAMES_IN_FLIGHT> tracyCtx;

    InternalSceneData active_scene_data;
    
    InternalSceneData internal_resources;
    std::unique_ptr<Scene> internal_scene;
    

    const uint32_t max_obj = 1000;
    const uint32_t max_mat = 1000;
    const uint32_t max_tex = 1000;
    const uint32_t max_light = 10;

    VkSampler textureSampler;

    // to be created
    std::array<vkBuffer, MAX_FRAMES_IN_FLIGHT> lightsBuffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> lightsBuffersMapped;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> shadowFrameBuffer;
    std::array<vkImage, MAX_FRAMES_IN_FLIGHT> shadowImages;
    VkRenderPass shadowRenderPass;
    ShaderHandle shadowShader_h;
    MaterialHandle shadowMaterial_h;
    VkExtent2D shadowSize = {.width = 1080, .height = 1080};

    std::vector<gbg::vkBuffer> globalBuffers;
    std::vector<void*> globalBuffersMapped;

    gbg::vkImage colorImage;

    gbg::vkImage depthImage;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

   private:
    void initVulkan();
    void initImgui();

    void initResources();

    void processScene();

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void createSwapChain();

    void recreateSwapChain();

    // the logical device is the abstract device that will be responsable for
    // reciving commands (like graphic commands). It is an interface with the
    // physical device
    void createLogicalDevice();

    void cleanupSwapChain();

    void createImageViews();

    void createColorResources();

    void createRenderPass();

    void createGlobalDescriptorSetLayouts();

    void createShadowResources();

    void createRendererObjects();

    void createFrameBuffers();

    void bindMaterial(VkCommandBuffer commandBuffer, MaterialHandle math, InternalSceneData& data); 

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

    void createMaterialDescriptorSet(MaterialHandle h, InternalSceneData& scene_data);
    void updateMaterialDescriptorSet(MaterialHandle h, InternalSceneData& scene_data);

    void createModelDescriptorSets();

    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);

    void recordDrawScene(VkCommandBuffer commandBuffer, VkViewport viewport, VkRect2D scissor, uint32_t imageIndex, SceneTreeHandle root, MaterialHandle override);


    void createSyncObjects();

    VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice pdevice);

    void updateGlobalDescriptorSets(uint32_t currentImage);

    void updateMesh(MeshHandle mesh_h, InternalSceneData& scene_data);
    void updateShader(ShaderHandle sh_h, InternalSceneData& scene_data, VkRenderPass renderPass, VkSampleCountFlagBits samples);
    void updateMaterial(MaterialHandle math, InternalSceneData& scene_data);
    void updateTexture(TextureHandle texture, InternalSceneData& scene_data);
    void updateLight(LightHandle lh, InternalSceneData& scene_data);

    void fillLightBuffer(uint32_t currentImage);
};
}  // namespace gbg
