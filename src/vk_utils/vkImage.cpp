#include "vkImage.hh"

#include <stdexcept>

#include "vkUtil.hh"
#include "vk_utils/vkCommandBuffer.hh"
#include "vk_utils/vkDevice.hh"

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

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(vkDevice device, VkImage image, VkFormat format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          uint32_t mipLevels) {
    VkCommandBuffer transBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(transBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    endSingleTimeCommands(device, transBuffer, device.transferCmdPool,
                          device.tqueue);
}

void copyBufferToImage(vkDevice device, VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    VkBufferImageCopy copyInfo{};
    copyInfo.bufferOffset = 0;
    copyInfo.bufferRowLength = 0;
    copyInfo.bufferImageHeight = 0;

    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.baseArrayLayer = 0;
    copyInfo.imageSubresource.layerCount = 1;
    copyInfo.imageSubresource.mipLevel = 0;

    copyInfo.imageExtent = {width, height, 1};
    copyInfo.imageOffset = {0, 0, 0};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

    gbg::endSingleTimeCommands(device, commandBuffer, device.transferCmdPool,
                               device.tqueue);
}

}  // namespace gbg
