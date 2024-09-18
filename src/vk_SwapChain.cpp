#include "vk_SwapChain.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cassert>
#include <stdexcept>

vk_SwapChain::vk_SwapChain(int width, int height, VkDevice device,
                           VkPhysicalDevice physicalDevice) {
    create(width, height, device, physicalDevice);
}

void vk_SwapChain::create(int width, int height, VkDevice device,
                          VkPhysicalDevice physicalDevice) {
    SwapChainSupportDetails swapChainSupport = querySupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        choosePresentMode(swapChainSupport.presentModes);
    VkExtent2D extent =
        chooseSwapExtent(swapChainSupport.capabilities, width, height);
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

    vkutils::QueueFamilyIndices indices =
        vkutils::findQueueFamilies(physicalDevice, surface);

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                     indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);

    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainImageExtent = extent;
}

vk_SwapChain::~vk_SwapChain() { cleanup(); }

VkFramebuffer vk_SwapChain::getFrameBuffer(int index) {
    assert(index >= swapChainFramebuffers.size());

    return swapChainFramebuffers.at(index);
}

VkImage vk_SwapChain::getImage(int index) {
    assert(index < swapChainImages.size());
    return swapChainImages.at(index);
}

VkFormat vk_SwapChain::getImageFormat() { return swapChainImageFormat; }
VkExtent2D vk_SwapChain::getImageExtent() { return swapChainImageExtent; }
VkSwapchainKHR vk_SwapChain::getSwapChain() { return swapChain; }

void vk_SwapChain::recreate(int width, int height, VkDevice device,
                            VkPhysicalDevice physicalDevice) {
    vkDeviceWaitIdle(device);

    cleanup();

    create(width, height, device, physicalDevice);
    createImageViews();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
}

vk_SwapChain::SwapChainSupportDetails vk_SwapChain::querySupport(
    VkPhysicalDevice physicalDevice) {
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

VkSurfaceFormatKHR vk_SwapChain::chooseFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB and
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR vk_SwapChain::choosePresentMode(
    const std::vector<VkPresentModeKHR>& presentModes) {
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D vk_SwapChain::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
    // if the window manager allows to have a bigger fame buffer than
    // the current window size, it points it by setting the
    // currentExtend to uint32_t limit.
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void vk_SwapChain::cleanup() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void vk_SwapChain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] =
            createImageView(swapChainImages[i], swapChainImageFormat,
                            VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}
void vk_SwapChain::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;
    createImage(swapChainImageExtent.width, swapChainImageExtent.height, 1,
                msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage,
                colorImageMemory);
    colorImageView =
        createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
void vk_SwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainImageExtent.width, swapChainImageExtent.height, 1,
                msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                depthImageMemory);
    depthImageView =
        createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}
void vk_SwapChain::createFrameBuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView, depthImageView, swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainImageExtent.width;
        framebufferInfo.height = swapChainImageExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
