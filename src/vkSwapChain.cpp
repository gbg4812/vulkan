#include "vkSwapChain.h"

#include <stdexcept>
#include <vector>

namespace gbg {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB and
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR choosePresentMode(
    const std::vector<VkPresentModeKHR>& presentModes) {
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t pmodesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &pmodesCount, nullptr);

    if (pmodesCount != 0) {
        details.presentModes.resize(pmodesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &pmodesCount, details.presentModes.data());
    }

    return details;
}

vkSwapChain createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device,
                            VkSurfaceKHR surface, VkExtent2D extent,
                            uint32_t graphicsFamily, uint32_t presentFamily) {
    vkSwapChain swapChain;

    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        choosePresentMode(swapChainSupport.presentModes);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 and
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
    if (graphicsFamily != presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;              // dont render cliped pixels
    createInfo.oldSwapchain = VK_NULL_HANDLE;  // if we change the window
                                               // size the swapchain must be
                                               // recreated, here we
                                               // especify that it won't be
                                               // handled for simplicity

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr,
                             &swapChain.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain.swapChain, &imageCount, nullptr);

    swapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain.swapChain, &imageCount,
                            swapChain.swapChainImages.data());
    swapChain.swapChainImageFormat = surfaceFormat.format;
    swapChain.swapChainImageExtent = extent;

    return swapChain;
}

void cleanupSwapChain(vkSwapChain& swapChain, VkDevice device) {
    for (auto imageView : swapChain.swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain.swapChain, nullptr);
}

}  // namespace gbg
