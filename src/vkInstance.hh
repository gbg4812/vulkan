#include <vulkan/vulkan_core.h>

#include <vector>
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
}  // namespace gbg
