#include "vkDevice.hh"

#include <vulkan/vulkan_core.h>

#include <set>

#include "Logger.h"
#include "vkInstance.hh"
namespace gbg {
vkDevice createDevice(VkPhysicalDevice pdevice,
                      const std::vector<const char*>& deviceExtensions,
                      bool enableValidationLayers,
                      const std::vector<const char*>& validationLayers,
                      VkSurfaceKHR surface) {
    // setup queue info struct to pass to the logical device creation who
    // will create the queues for us

    auto gfamily = getGraphicQueueFamilyIndex(pdevice);
    auto pfamily = getPresentQueueFamilyIndex(pdevice, surface);
    auto tfamily = getTransferQueueFamilyIndex(pdevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {gfamily.value(), pfamily.value(),
                                              tfamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(pdevice, &deviceFeatures);
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    vkDevice device;
    device.pdevice = pdevice;
    if (vkCreateDevice(device.pdevice, &deviceCreateInfo, nullptr,
                       &device.ldevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device.ldevice, gfamily.value(), 0, &device.gqueue);
    vkGetDeviceQueue(device.ldevice, pfamily.value(), 0, &device.pqueue);
    vkGetDeviceQueue(device.ldevice, tfamily.value(), 0, &device.tqueue);
    return device;
}
}  // namespace gbg
