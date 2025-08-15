#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "vkDevice.hh"
namespace gbg {
struct vkInstance {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    bool enableValidationLayers;
};

vkInstance createInstance(const std::vector<const char*>& validationLayers,
                          bool enableValidationLayers);

void destoryInstance(vkInstance instance);

void setupDebugMessenger(vkInstance instance, bool enableValidationLayers);

std::optional<uint32_t> getGraphicQueueFamilyIndex(VkPhysicalDevice pdevice);
std::optional<uint32_t> getTransferQueueFamilyIndex(VkPhysicalDevice pdevice);
std::optional<uint32_t> getPresentQueueFamilyIndex(VkPhysicalDevice pdevice,
                                                   VkSurfaceKHR surface);

bool getDeviceQueueCompatibility(VkPhysicalDevice device, VkSurfaceKHR surface);

}  // namespace gbg
