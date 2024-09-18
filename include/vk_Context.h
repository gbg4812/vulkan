#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Logger.h"

#pragma once

class vk_Context {
   public:
    static bool init(std::vector<const char*> extensions,
                     std::vector<const char*> validationLayers,
                     bool validationLayersEnabled) {
        if (instanceCreated) {
            throw std::logic_error("Vulkan context initialized twice!");
        }

        if (validationLayersEnabled &&
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

        LOG(extensions)

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (validationLayersEnabled) {
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

        if (vkCreateInstance(&createInfo, nullptr, instance.get()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

        instanceCreated = true;
    }

    static std::shared_ptr<VkInstance> getInstance() {
        if (!instanceCreated) {
            throw std::logic_error("Instance not initialized!");
        }
        return instance;
    }

    vk_Context() = delete;

    static VkPhysicalDevice getPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with vulkan suport!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        std::multimap<uint32_t, VkPhysicalDevice> scoredDevices;
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        for (const auto& device : devices) {
            uint32_t score = deviceScore(device);
            if (score) {
                scoredDevices.emplace(score, device);
            }
        }

        if (scoredDevices.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        physicalDevice = scoredDevices.rbegin()->second;
        msaaSamples = getMaxUsableSampleCount();
    }

   private:
    static bool instanceCreated;
    static std::shared_ptr<VkInstance> instance;

    static bool checkValidationLayerSupport(
        std::vector<const char*> validationLayers) {
        uint32_t layerCount;
        // geting the number of available layers
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);

        // getting the list of available layers propertyes to know if
        // all the requested layers are present needed.
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const std::string& layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (std::strcmp(layerName.c_str(), layerProperties.layerName) ==
                    0) {
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
    uint32_t deviceScore(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        uint32_t score = 100;
        if (deviceProperties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 10;
        }
        if (deviceFeatures.geometryShader) {
            score += 1;
        }
        QueueFamilyIndices indices = findQueueFamilies(device);

        if (indices.isComplete() and checkDeviceExtensionSupport(device)) {
            SwapChainSupportDetails swapChainDetails =
                querySwapChainSupport(device);
            if (swapChainDetails.formats.empty() or
                swapChainDetails.presentModes.empty()) {
                score = 0;
            }

        } else {
            score = 0;
        }

        return score;
}


    // static method since it is not concrete to the application instance
    // checks if signature is correct
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage
                  << std::endl;

        return VK_FALSE;
    }

    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        // All types are followed by EXT because they are part of the
        // VK_EXT_DEBUG_UTILS_EXT_NAME extension
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // types of severity that will be handled by the messenger
        // other way:
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT &
        //     !VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // types of messanges "                                "
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        // the callback to call when a mesa
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
};

bool vk_Context::instanceCreated = false;
