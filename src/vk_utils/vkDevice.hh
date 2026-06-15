#pragma once
#include <vulkan/vulkan_core.h>

#include <vector>
namespace gbg {
struct vkDevice {
    VkDevice ldevice;
    VkPhysicalDevice pdevice;
    VkQueue gqueue;
    VkQueue pqueue;
    VkQueue tqueue;
    VkCommandPool graphicsCmdPool;
    VkCommandPool transferCmdPool;
};
vkDevice createDevice(VkPhysicalDevice pdevice,
                      const std::vector<const char*>& deviceExtensions,
                      VkSurfaceKHR surface);

}  // namespace gbg
