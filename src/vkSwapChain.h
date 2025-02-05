
#ifndef GBG_SWAPCHAIN
#define GBG_SWAPCHAIN

#include <vulkan/vulkan_core.h>

#include <vector>

namespace gbg {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct vkSwapChain {
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainImageExtent;
    std::vector<VkImageView> swapChainImageViews;
};

vkSwapChain createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device,
                            VkSurfaceKHR surface, VkExtent2D extent,
                            uint32_t graphicsFamily, uint32_t presentFamily);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface);

void cleanupSwapChain(vkSwapChain& swapChain, VkDevice device);

}  // namespace gbg
#endif
