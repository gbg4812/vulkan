#include <vulkan/vulkan.h>

namespace vkutils {
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags flags);

}  // namespace vkutils
