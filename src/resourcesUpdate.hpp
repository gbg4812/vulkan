#pragma once

#include "InternalSceneData.hpp"
#include "vk_utils/vkRenderPass.hpp"

namespace gbg {

enum SObjFlags : gbg::DependencyMask {
    NONE = 0,
    NEW = 1,
    DELETED = 1 << 1,
    ALL = std::numeric_limits<gbg::DependencyMask>::max(),

    // SHADER FLAGS
    DIRTY_SHADER_CODE = 1 << 2,

    // MATERIAL FLAGS
    DIRTY_PARAMETER = 1 << 2,
    SHADER_CHANGED = 1 << 3,
    TEXTURE_CHANGED = 1 << 4,
};

void cleanShaderVkResources(const vkDevice& device, srShader& sr_sh);

void createShaderVkResources(
    vkDevice device, Shader& shader, srShader& sr_sh, vkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> rendererDescriptorSetLayouts);

void createMeshVkResources(vkDevice device, MeshHandle mesh_h,
                           InternalSceneData& scene_data);

void cleanMaterialVkResources(const vkDevice& device,
                              VkDescriptorPool materialDescPool,
                              srMaterial& srmt);

void createMaterialVkResources(vkDevice device, MaterialHandle math,
                               InternalSceneData& scene_data,
                               VkDescriptorPool materialDescPool);

void updateParameterValues(const vkDevice& device, Material& mat,
                           srMaterial& srmt);

void updateShader(
    vkDevice device, ShaderHandle sh_h, InternalSceneData& scene_data,
    vkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> rendererDescriptorSetLayouts);

void updateMesh(vkDevice device, MeshHandle mesh_h,
                InternalSceneData& scene_data);

void updateMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                 InternalSceneData& scene_data,
                                 VkSampler textureSampler);

void createMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                 InternalSceneData& scene_data,
                                 VkDescriptorPool materialDescPool);
void updateMaterial(vkDevice device, MaterialHandle math,
                    InternalSceneData& scene_data,
                    VkDescriptorPool materialDescPool,
                    VkSampler textureSampler);

void updateTexture(vkDevice device, TextureHandle h,
                   InternalSceneData& scene_data, VkSampler textureSampler);
}  // namespace gbg
