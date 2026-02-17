#pragma once
#include <vulkan/vulkan_core.h>

#include "vkDevice.hh"
namespace gbg {
VkCommandBuffer beginSingleTimeCommands(vkDevice device,
                                        VkCommandPool commandPool);
void endSingleTimeCommands(vkDevice device, VkCommandBuffer commandBuffer,
                           VkCommandPool commandPool, VkQueue queue);
}  // namespace gbg
