
#pragma once
#include <vulkan/vulkan_core.h>

#include "vk_utils/vkPipeline.hh"

namespace gbg {
struct srShader {
    vkPipeline pipeline;
    VkDescriptorSetLayout layout;
};

void destroySrShader(const vkDevice& device, const srShader& shader);

}  // namespace gbg
