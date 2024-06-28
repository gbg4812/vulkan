#include <stdexcept>
#include <vector>

#include "vk_utils.h"

uint32_t vkutils::findMemoryType(VkPhysicalDevice physicalDevice,
                                 uint32_t typeFilter,
                                 VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) and
            (memProperties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

vkutils::QueueFamilyIndices vkutils::findQueueFamilies(VkPhysicalDevice device,
                                                       VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }

        if (indices.isComplete()) break;
        i++;
    }
    return indices;
}

void vkutils::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                         VkDeviceSize size) {
    VkCommandBuffer cpyCmdBuffer = beginSingleTimeCommands(transferCmdPool);

    VkBufferCopy cpyRegion{};
    cpyRegion.srcOffset = 0;
    cpyRegion.dstOffset = 0;
    cpyRegion.size = size;

    vkCmdCopyBuffer(cpyCmdBuffer, srcBuffer, dstBuffer, 1, &cpyRegion);

    endSingleTimeCommands(cpyCmdBuffer, transferCmdPool, transferQueue);
}

VkFormat vkutils::findSupportedFormats(const std::vector<VkFormat>& candidates,
                                       VkImageTiling tiling,
                                       VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat vkutils::findDepthFormat() {
    return findSupportedFormats(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool vkutils::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}
