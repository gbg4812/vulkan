#ifndef GBG_VKIMAGE
#define GBG_VKIMAGE

#include <vulkan/vulkan_core.h>

#include <optional>
#include <vector>

namespace gbg {

struct vkImage {
    VkImage image;
    VkDeviceMemory memory;
    std::optional<VkImageView> view;
};

vkImage createImage(VkPhysicalDevice physicalDevice, VkDevice device,
                    uint32_t width, uint32_t height, uint32_t mipLevels,
                    VkSampleCountFlagBits numSamples, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties);

void addImageView(vkImage& image, VkDevice device, VkFormat format,
                  VkImageAspectFlags aspectFlags, uint32_t mipLevels);

VkImageView createImageView(VkImage image, VkDevice device, VkFormat format,
                            VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void destoryImage(vkImage image, VkDevice device);

}  // namespace gbg

#endif
