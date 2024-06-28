#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

#pragma once

namespace vkutils {
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;
    bool isComplete() {
        return graphicsFamily.has_value() and presentFamily.has_value() and
               transferFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags flags);

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

VkFormat findSupportedFormats(const std::vector<VkFormat>& candidates,
                              VkImageTiling tiling,
                              VkFormatFeatureFlags features);

VkFormat findDepthFormat();

bool hasStencilComponent(VkFormat format);

}  // namespace vkutils
