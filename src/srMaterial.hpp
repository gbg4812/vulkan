#pragma once
#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "Resource.hpp"
#include "vk_utils/vkBuffer.hh"

namespace gbg {

struct srParameterValues {
    unsigned char* data;
    size_t size;
};

struct srMaterial : public Resource {
    srMaterial(std::string name, uint32_t rid) : Resource(name, rid){};
    VkDescriptorSet descriptor_set;
    vkBuffer paramBuffer;
    srParameterValues parameter_values;
};

struct srMaterialHandle : public ResourceHandle {
    srMaterialHandle() : ResourceHandle(){};
    srMaterialHandle(uint32_t rid, size_t index) : ResourceHandle(rid, index){};
};

srParameterValues allocateParameterValues(Material& model);

void destroySrMaterial(const vkDevice& device, const srMaterial& mat);

}  // namespace gbg
