#include "srMaterial.hpp"

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "ParameterTypes.hpp"
#include "traits/traits.hpp"
#include "vk_utils/vkBuffer.hh"
namespace gbg {

void fillParameterValues(Material& material, srMaterial& srmat) {
    const auto& values = material.getValues();
    // copy
    size_t addr_offset = 0;
    for (auto& value : values) {
        size_t val_size = std::visit<size_t>(
            overloads{[](TextureHandle handle) { return 0; },
                      [](auto&& val) { return sizeof(decltype(val)); },
                      [](const vec3_t& val) { return sizeof(float_t) * 3; }},
            value);
        size_t padd = 0;
        if (val_size == 0) continue;
        if (addr_offset % val_size) padd = val_size - (addr_offset % val_size);
        addr_offset += padd;
        std::memcpy(srmat.values.data + addr_offset, &value, val_size);
        addr_offset += val_size;

        if (addr_offset > srmat.values.size)
            throw std::logic_error("Writting out of parameters block!");
    }
}

srParameterValues allocateParameterValues(Material& material) {
    const auto& values = material.getValues();

    // compute size;
    // typedef std::variant<int32_t, float_t, vec2_t, vec3_t, TextureHandle>
    // parm_vt;
    srParameterValues parm_values{nullptr, 0};
    for (auto& value : values) {
        size_t val_size = std::visit<size_t>(
            overloads{
                [](TextureHandle handle) -> size_t { return 0; },
                [](const int32_t& val) -> size_t { return sizeof(int32_t); },
                [](const float_t& val) -> size_t { return sizeof(float_t); },
                [](const vec2_t& val) -> size_t { return sizeof(vec2_t); },
                [](const vec3_t& val) -> size_t {
                    return sizeof(float_t) * 3;
                }},
            value);
        size_t padd = 0;
        if (val_size == 0) continue;
        if (parm_values.size % val_size)
            padd = val_size - (parm_values.size % val_size);
        parm_values.size += padd;
        parm_values.size += val_size;
    }

    // allocate
    parm_values.data = (unsigned char*)std::aligned_alloc(16, parm_values.size);

    if (parm_values.data == nullptr) {
        throw std::runtime_error("Failed to allocate parameter values!");
    }
    return parm_values;
}

void createTextureDescriptors(const vkDevice& device, srMaterial& sr_material,
                              gbg::Material& material, VkDescriptorPool pool,
                              std::vector<VkDescriptorSetLayout> layouts) {
    const auto& values = material.getValues();

    assert(sr_material.texture_descriptors.size() == 0);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorSetCount = layouts.size();
    allocateInfo.descriptorPool = pool;
    allocateInfo.pSetLayouts = layouts.data();

    if (!vkAllocateDescriptorSets(device.ldevice, &allocateInfo,
                                  sr_material.texture_descriptors.data())) {
        throw std::runtime_error(
            "Filed to create material texture descriptor set");
    }
}

void destroySrMaterial(const vkDevice& device, const srMaterial& mat) {
    destroyBuffer(device, mat.paramBuffer);
}

}  // namespace gbg
