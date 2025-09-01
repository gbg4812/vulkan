#pragma once
#include <vulkan/vulkan_core.h>

#include <optional>

#include "vkCommandBuffer.hh"
#include "vkDevice.hh"
#include "vkUtil.hh"

namespace gbg {
struct vkBuffer {
    VkBuffer buffer;
    VkDeviceSize size;
    VkDeviceMemory memory;
    std::optional<VkBufferView> view;
};

vkBuffer createBuffer(vkDevice device, VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);

void copyBuffer(vkDevice device, vkBuffer srcBuffer, vkBuffer dstBuffer);

void destroyBuffer(vkDevice device, vkBuffer buffer);
}  // namespace gbg
