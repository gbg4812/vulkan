#pragma once
#include <vulkan/vulkan_core.h>

#include "vk_utils/vkBuffer.hh"

namespace gbg {
struct srModel {
    VkDescriptorSet mdlDescSet;
    vkBuffer mdlBuffer;
};

}  // namespace gbg
