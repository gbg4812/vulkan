
#include "vkBuffer.hh"

#include <vulkan/vulkan_core.h>

#include <stdexcept>

namespace gbg {
vkBuffer createBuffer(vkDevice device, VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;

    vkBuffer buffer;
    buffer.size = size;

    if (vkCreateBuffer(device.ldevice, &bufferInfo, nullptr, &buffer.buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device.ldevice, buffer.buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(
        device.pdevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device.ldevice, &allocateInfo, nullptr,
                         &buffer.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device.ldevice, buffer.buffer, buffer.memory, 0);
    return buffer;
}

void copyBuffer(vkDevice device, vkBuffer srcBuffer, vkBuffer dstBuffer) {
    VkCommandBuffer cpyCmdBuffer =
        beginSingleTimeCommands(device, device.transferCmdPool);

    VkBufferCopy cpyRegion{};
    cpyRegion.srcOffset = 0;
    cpyRegion.dstOffset = 0;
    cpyRegion.size = srcBuffer.size;

    vkCmdCopyBuffer(cpyCmdBuffer, srcBuffer.buffer, dstBuffer.buffer, 1,
                    &cpyRegion);

    endSingleTimeCommands(device, cpyCmdBuffer, device.transferCmdPool,
                          device.tqueue);
}

void destroyBuffer(vkDevice device, vkBuffer buffer) {
    vkDestroyBuffer(device.ldevice, buffer.buffer, nullptr);
    vkFreeMemory(device.ldevice, buffer.memory, nullptr);
}
}  // namespace gbg
