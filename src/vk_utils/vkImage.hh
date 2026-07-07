#ifndef GBG_VKIMAGE
#define GBG_VKIMAGE

#include "vkDevice.hh"
#include <vulkan/vulkan_core.h>

#include <optional>

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

bool hasStencilComponent(VkFormat format);


void transitionImageLayout(vkDevice device, VkCommandBuffer transBuffer, VkImage image, VkFormat format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          uint32_t mipLevels);

void copyBufferToImage(vkDevice device, VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height);

}  // namespace gbg

#endif
