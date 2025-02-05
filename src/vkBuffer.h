
#ifndef GBG_VKBUFFER
#define GBG_VKBUFFER

#include <vulkan/vulkan_core.h>

#include <optional>
namespace gbg {
struct vkBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    std::optional<VkBufferView> view;
};

vkBuffer createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                      VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);
}  // namespace gbg
#endif
