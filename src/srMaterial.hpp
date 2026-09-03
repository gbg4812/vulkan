#pragma once
#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "Resource.hpp"
#include "macros.hpp"
#include "vk_utils/vkBuffer.hh"

namespace gbg {

struct srParameterValues {
    unsigned char* data = nullptr;
    size_t size = 0;
};

struct srMaterial : public Resource<MaterialHandle> {
    RESOURCE_CONSTR(srMaterial)

    VkDescriptorSet descriptor_set;
    std::vector<VkDescriptorSet> texture_descriptors;
    vkBuffer paramBuffer;
    srParameterValues values;
};

RELATED_RESOURCE_MANAGER(srMaterial, MaterialHandle);

void fillParameterValues(Material& material, srMaterial& srmat);

srParameterValues allocateParameterValues(Material& model);

void createTextureDescriptors(const vkDevice& device, srMaterial& mat);

void destroySrMaterial(const vkDevice& device, const srMaterial& mat);

}  // namespace gbg
