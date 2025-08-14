#include "vkInstance.hh"

#include <cstring>
#include <stdexcept>
#include <vector>

#include "GLFW/glfw3.h"
#include "vkDebugMessangerEXT.hh"

namespace gbg {

const std::vector<const char*> getRequiredExtensions(
    bool enableValidationLayers) {
    // get the extensions required by glfw
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    // pointer to extension list
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // iterate from extension list address until the last address
    std::vector<const char*> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);

    // enable the messenger extension to print the layers logging if
    // validation layers are enabled
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
bool checkValidationLayerSupport(
    const std::vector<const char*>& validationLayers) {
    uint32_t layerCount;
    // geting the number of available layers
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);

    // getting the list of available layers propertyes to know if
    // all the requested layers are present needed.
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void setupDebugMessenger(vkInstance instance, bool enableValidationLayers) {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(instance.instance, &createInfo, nullptr,
                                     &instance.debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger");
    }
}

vkInstance createInstance(const std::vector<const char*>& validationLayers,
                          bool enableValidationLayers) {
    if (enableValidationLayers &&
        !checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions(enableValidationLayers);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    vkInstance instance{};

    if (vkCreateInstance(&createInfo, nullptr, &instance.instance) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    instance.enableValidationLayers = enableValidationLayers;

    setupDebugMessenger(instance, enableValidationLayers);
    return instance;
}

void destoryInstance(vkInstance instance) {
    if (instance.enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance.instance,
                                      instance.debugMessenger, nullptr);
    }
    vkDestroyInstance(instance.instance, nullptr);
}

std::optional<uint32_t> getGraphicQueueFamilyIndex(VkPhysicalDevice pdevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return std::optional<uint32_t>(i);
        }
        i++;
    }

    return std::optional<uint32_t>();
}
std::optional<uint32_t> getTransferQueueFamilyIndex(VkPhysicalDevice pdevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            return std::optional<uint32_t>(i);
        }
        i++;
    }

    return std::optional<uint32_t>();
}

std::optional<uint32_t> getPresentQueueFamilyIndex(VkPhysicalDevice pdevice,
                                                   VkSurfaceKHR surface) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount,
                                             queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, i, surface,
                                             &presentSupport);
        if (presentSupport) {
            return std::optional<uint32_t>(i);
        }
        i++;
    }

    return std::optional<uint32_t>();
}

}  // namespace gbg
