#pragma once
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkInstance.hh"

namespace gbg {
struct RendererContext {
    vkInstance instance;
    VkSurfaceKHR surface;
    vkDevice device;
    int width;
    int height;
};
}  // namespace gbg
