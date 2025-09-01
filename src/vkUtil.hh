#pragma once
#include <vulkan/vulkan_core.h>

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags flags);
