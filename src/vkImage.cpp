#include "vkImage.h"

#include <stdexcept>

#include "vkUtil.hh"

namespace gbg {

vkImage createImage(VkPhysicalDevice physicalDevice, VkDevice device,
                    uint32_t width, uint32_t height, uint32_t mipLevels,
                    VkSampleCountFlagBits numSamples, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties) {
    vkImage image;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = tiling;

    if (vkCreateImage(device, &imageInfo, nullptr, &image.image) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements imageRequirements;
    vkGetImageMemoryRequirements(device, image.image, &imageRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physicalDevice, imageRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image.memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("filed to allocate image!");
    }

    vkBindImageMemory(device, image.image, image.memory, 0);

    return image;
}

VkImageView createImageView(VkImage image, VkDevice device, VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    // image porpouse
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    // the layers are for example to create stereographic images with
    // an image for each eye
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView view;
    if (vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
    }
    return view;
}

void addImageView(vkImage& image, VkDevice device, VkFormat format,
                  VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    image.view =
        createImageView(image.image, device, format, aspectFlags, mipLevels);
}

void destoryImage(vkImage image, VkDevice device) {
    if (image.view.has_value())
        vkDestroyImageView(device, image.view.value(), nullptr);
    vkDestroyImage(device, image.image, nullptr);
    vkFreeMemory(device, image.memory, nullptr);
}

}  // namespace gbg
