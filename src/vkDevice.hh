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
};
vkDevice createDevice(VkPhysicalDevice pdevice,
                      const std::vector<const char*>& deviceExtensions,
                      bool enableValidationLayers,
                      const std::vector<const char*>& validationLayers,
                      VkSurfaceKHR surface);

}  // namespace gbg
