#pragma once
#include <vulkan/vulkan_core.h>

#include <optional>

namespace gbg {
struct vkBuffer {
    VkBuffer buffer;
    VkDeviceSize size;
    VkDeviceMemory memory;
    std::optional<VkBufferView> view;
};

vkBuffer createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                      VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
}  // namespace gbg
