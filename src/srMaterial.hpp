#pragma once
#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "vk_utils/vkBuffer.hh"

namespace gbg {

struct srParameterValues {
    unsigned char* data;
    size_t size;
};

struct srMaterial {
    VkDescriptorSet descriptor_set;
    vkBuffer paramBuffer;
    srParameterValues parameter_values;
};

srParameterValues allocateParameterValues(Material& model);

void destroySrMaterial(const vkDevice& device, const srMaterial& mat);

}  // namespace gbg
