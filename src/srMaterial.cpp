#include "srMaterial.hpp"

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "traits/traits.hpp"
#include "vk_utils/vkBuffer.hh"
namespace gbg {

srParameterValues allocateParameterValues(Material& material) {
    const auto& values = material.getValues();

    // compute size;
    srParameterValues parm_values{nullptr, 0};
    for (auto& value : values) {
        std::visit(overloads{
                       [&parm_values](TextureHandle handle) {

                       },
                       [&parm_values](auto&& val) {
                           size_t val_size = sizeof(decltype(val));
                           size_t padd = 0;
                           if (parm_values.size % val_size)
                               padd = val_size - (parm_values.size % val_size);
                           parm_values.size += padd;
                           parm_values.size += val_size;
                       },
                   },
                   value);
    }  // namespace gbg

    // allocate
    parm_values.data = new uint8_t[parm_values.size];

    if (parm_values.data == nullptr) {
        throw std::runtime_error("Failed to allocate parameter values!");
    }

    //
    // copy
    size_t addr_offset = 0;
    for (auto& value : values) {
        std::visit(overloads{
                       [&parm_values](TextureHandle handle) {

                       },
                       [&](const auto& val) {
                           size_t val_size = sizeof(decltype(val));
                           size_t padd = 0;
                           if (addr_offset % val_size)
                               padd = val_size - (addr_offset % val_size);
                           addr_offset += padd;
                           std::memcpy((parm_values.data + addr_offset), &val,
                                       val_size);
                           addr_offset += val_size;
                       },
                   },
                   value);
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
    delete[] mat.parameter_values.data;
    destroyBuffer(device, mat.paramBuffer);
}

}  // namespace gbg
