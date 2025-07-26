#ifndef GBG_VKUTIL
#define GBG_VKUTIL

#include <vulkan/vulkan_core.h>

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags flags);

#endif  // !GBG_VKUTIL
