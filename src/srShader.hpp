
#pragma once
#include <vulkan/vulkan_core.h>

#include "Resource.hpp"
#include "vk_utils/vkPipeline.hh"

namespace gbg {
struct srShader : public Resource {
    srShader(std::string name, uint32_t rid) : Resource(name, rid) {}
    vkPipeline pipeline;
    VkDescriptorSetLayout layout;
};

struct srShaderHandle : public ResourceHandle {
    srShaderHandle() : ResourceHandle(){};
    srShaderHandle( uint32_t rid, size_t index) : ResourceHandle(rid, index){};
};

void destroySrShader(const vkDevice& device, const srShader& shader);

}  // namespace gbg
