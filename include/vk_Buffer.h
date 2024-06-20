#include <vulkan/vulkan.h>

#include <stdexcept>

#include "vkutils.h"
#include "vulkan/vulkan_core.h"

#pragma once

struct vk_Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;

    vk_Buffer() {
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    }

    vk_Buffer(VkDevice device, VkPhysicalDevice physicalDevice,
              VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties) {
        this->device = device;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.flags = 0;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = vkutils::findMemoryType(
            physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, memory, 0);
    }

    void destory() {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
        }

        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }

   private:
    VkDevice device;
};
