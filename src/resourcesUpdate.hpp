#pragma once

#include "InternalSceneData.hpp"
#include "vk_utils/vkRenderPass.hpp"

namespace gbg {

/*  idiom:
 *  - *_M modified (the resource taged needs update and the dependents also)
 */

enum SObjFlags : gbg::DependencyMask {
    NONE = 0,
    NEW = 1,
    DELETED = 1 << 1,
    ALL = std::numeric_limits<gbg::DependencyMask>::max(),

    // SHADER FLAGS
    CODE_M = 1 << 2,

    // MATERIAL FLAGS
    PARAMETER_VALUE_M = 1 << 2,
    PARAMETER_INTERFACE_M = 1 << 3,
    TEXTURE_PARAMETER_VALUE_M = 1 << 4,
};

void cleanShaderVkResources(const vkDevice& device, srShader& sr_sh);

void createShaderVkResources(
    vkDevice device, Shader& shader, srShader& sr_sh, vkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> rendererDescriptorSetLayouts,
    std::vector<VkPushConstantRange> push_constant_ranges);

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
