
#pragma once
#include <vulkan/vulkan_core.h>

namespace gbg {
struct srShader {
    VkPipeline pipeline;
    VkDescriptorSetLayout layout;
};

}  // namespace gbg
