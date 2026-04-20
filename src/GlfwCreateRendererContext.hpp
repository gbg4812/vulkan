#pragma once

#include <vulkan/vulkan_core.h>

#include <map>
#include <set>
#include <stdexcept>

#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkInstance.hh"
#include "vk_utils/vkSwapChain.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "RendererContext.hpp"

namespace gbg {
inline bool checkDeviceExtensionSupport(
    VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

inline uint32_t deviceScore(VkPhysicalDevice device, VkSurfaceKHR surface,
                            const std::vector<const char*> deviceExtensions) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score = 100;
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 10;
    }
    if (deviceFeatures.geometryShader) {
        score += 1;
    }

    if (getDeviceQueueCompatibility(device, surface) and
        checkDeviceExtensionSupport(device, deviceExtensions)) {
        gbg::SwapChainSupportDetails swapChainDetails =
            gbg::querySwapChainSupport(device, surface);
        if (swapChainDetails.formats.empty() or
            swapChainDetails.presentModes.empty()) {
            score = 0;
        }

    } else {
        score = 0;
    }

    return score;
}

inline VkPhysicalDevice pickPhysicalDevice(
    vkInstance instance, VkSurfaceKHR surface,
    const std::vector<const char*>& deviceExtensions) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with vulkan suport!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    std::multimap<uint32_t, VkPhysicalDevice> scoredDevices;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, devices.data());
    for (const auto& pdevice : devices) {
        uint32_t score = deviceScore(pdevice, surface, deviceExtensions);
        if (score) {
            scoredDevices.emplace(score, pdevice);
        }
    }

    if (scoredDevices.empty()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    return scoredDevices.rbegin()->second;
}

inline RendererContext glfwCreateRendererContext(
    GLFWwindow* window, const std::vector<const char*>& validationLayers,
    bool enableValidationLayers,
    const std::vector<const char*>& deviceExtensions) {
    RendererContext context{};
    context.instance = createInstance(validationLayers, enableValidationLayers);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(context.instance.instance, window, nullptr,
                                &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    context.surface = surface;

    // the logical device is the abstract device that will be responsable for
    // reciving commands (like graphic commands). It is an interface with the
    // physical device
    VkPhysicalDevice physicalDevice =
        pickPhysicalDevice(context.instance, surface, deviceExtensions);
    vkDevice device =
        createDevice(physicalDevice, deviceExtensions, enableValidationLayers,
                     validationLayers, surface);
    context.device = device;
    glfwGetFramebufferSize(window, &context.width, &context.height);
    return context;
}
}  // namespace gbg
