
#pragma once
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string_view>
#include <vector>

#include "Shader.hpp"
#include "vk_utils/vkDevice.hh"

namespace gbg {
struct vkVertexInputDescription {
    VkVertexInputBindingDescription binding_desc;
    VkVertexInputAttributeDescription attrib_desc;
};

struct vkPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout;
};

struct vkShaderStage {
    VkShaderModule module;
    VkShaderStageFlagBits stage;
};

vkVertexInputDescription getVertexVector3InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexVector2InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexFloatInputDescription(uint32_t attrib_id);

vkPipeline createGraphicsPipeline(
    const vkDevice& device, const std::vector<vkShaderStage>& shaderStages,
    const std::vector<VkDescriptorSetLayout>& desc_sets_layouts,
    const std::vector<VkVertexInputBindingDescription>& binding_desc,
    const std::vector<VkVertexInputAttributeDescription>& attrib_desc,
    const std::vector<VkPushConstantRange>& push_constants,
    VkSampleCountFlagBits msaaSamples, VkRenderPass renderPass,
    VkPrimitiveTopology topology);

VkShaderModule createShaderModule(const vkDevice& device,
                                  const std::vector<uint32_t>& code);

}  // namespace gbg
