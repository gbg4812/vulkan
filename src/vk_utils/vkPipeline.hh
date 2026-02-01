
#pragma once
#include <vulkan/vulkan_core.h>

#include <string_view>
#include <vector>

namespace gbg {
struct vkVertexInputDescription {
    VkVertexInputBindingDescription binding_desc;
    VkVertexInputAttributeDescription attrib_desc;
};

vkVertexInputDescription getVertexVector3InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexVector2InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexFloatInputDescription(uint32_t attrib_id);

VkPipeline createGraphicsPipeline(
    const VkDevice& device, std::string_view vertShaderCode,
    std::string_view fragShaderCode,
    const std::vector<VkDescriptorSetLayout>& desc_sets_layouts,
    const std::vector<VkVertexInputBindingDescription>& binding_desc,
    const std::vector<VkVertexInputAttributeDescription>& attrib_desc,
    VkSampleCountFlagBits msaaSamples, VkRenderPass renderPass);

VkShaderModule createShaderModule(const std::vector<char>& code);

}  // namespace gbg
