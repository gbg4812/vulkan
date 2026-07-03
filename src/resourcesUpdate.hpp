

#include "InternalSceneData.hpp"
#include "vk_utils/vkRenderPass.hpp"

namespace gbg {

void updateShader(vkDevice device, ShaderHandle sh_h,
                  InternalSceneData& scene_data, vkRenderPass renderPass,
                  VkDescriptorSetLayout globalDescriptorSetLayout);

void updateMesh(vkDevice device, MeshHandle mesh_h,
                               InternalSceneData& scene_data);

void updateMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                                InternalSceneData& scene_data, VkSampler textureSampler);

void createMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                                InternalSceneData& scene_data, VkDescriptorPool materialDescPool);
void updateMaterial(vkDevice device, MaterialHandle math,
                                   InternalSceneData& scene_data, VkDescriptorPool materialDescPool, VkSampler textureSampler);

void updateTexture(vkDevice device, TextureHandle h,
                                  InternalSceneData& scene_data, VkSampler textureSampler);
}
