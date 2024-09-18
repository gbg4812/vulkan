#include "vk_Context.h"
#include "vk_SwapChain.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

int main() {
    auto extensions = vk_SwapChain::getRequiredExtensions();
    vk_Context::init(extensions + deviceExtensions, validationLayers, true);
    vk_SwapChain sc;
}
