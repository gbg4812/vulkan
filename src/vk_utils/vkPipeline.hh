
#pragma once
#include <vulkan/vulkan_core.h>

namespace gbg {
struct vkVertexInputDescription {
    VkVertexInputBindingDescription binding_desc;
    VkVertexInputAttributeDescription attrib_desc;
};

vkVertexInputDescription getVertexVector3InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexVector2InputDescription(uint32_t attrib_id);
vkVertexInputDescription getVertexFloatInputDescription(uint32_t attrib_id);
}  // namespace gbg
