#pragma once
#include <vulkan/vulkan_core.h>

#include <unordered_map>

#include "InternalSceneData.hpp"
#include "MaterialFunctions.hpp"
#include "PerObjectPushConstant.hpp"
#include "loaders/objLoader.hpp"
#include "macros.hpp"
#include "resourcesUpdate.hpp"
#include "shaderReflexion.hpp"
#include "srMaterial.hpp"
#include "vk_utils/vkRenderPass.hpp"

namespace gbg {
inline void loadRendererResources(
    vkDevice device, VkDescriptorSetLayout globalDescSet,
    InternalSceneData& internal_resources,
    const std::unordered_map<std::string, vkRenderPass>& renderPasses,
    VkDescriptorPool materialDescPool, VkSampler textureSampler) {
    Scene* sc = internal_resources.scene;

    auto& ms_mg = sc->getMeshManager();
    auto& mt_mg = sc->getMaterialManager();
    auto& sh_mg = sc->getShaderManager();

    auto col_sh_h = sh_mg.create("PlainColorShader");
    auto& col_sh = sh_mg.get(col_sh_h);
    setShaderCode(col_sh, "data/models/RendererResources/plain_color.vert",
                  VERTEX);
    setShaderCode(col_sh, "data/models/RendererResources/plain_color.frag",
                  FRAGMENT);
    reflectShader(col_sh);
    col_sh.topology = LINES;

    auto white_mt_h = mt_mg.create("White Material");
    auto& white_mt = mt_mg.get(white_mt_h);
    white_mt.setShader(col_sh_h);
    setParametersFromShader(*sc, white_mt);
    white_mt.setParameterValue<VEC3_PARM>(0, glm::vec3(1.0f, 1.0f, 1.0f));

    objLoader("data/models/RendererResources/RendererObjects.obj", sc, sc->root,
              white_mt_h);

    for (auto msh_h : ms_mg) {
        createMeshVkResources(device, msh_h, internal_resources);
    }

    col_sh.shadow = false;

    VkPushConstantRange pushConstants{};
    pushConstants.offset = 0;
    pushConstants.size = sizeof(PerObjectPushConstant);
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    auto col_srsh_h = internal_resources.srsh_mg.create("srColorShader");
    auto& col_srsh = internal_resources.srsh_mg.get(col_srsh_h);
    createShaderVkResources(device, col_sh, col_srsh, renderPasses.at("color"),
                            {globalDescSet}, {pushConstants});

    auto col_srmt_h = internal_resources.srmat_mg.create("srWhiteMaterial");
    auto& col_srmt = internal_resources.srmat_mg.get(col_srmt_h);
    createMaterialVkResources(device, white_mt_h, internal_resources,
                              materialDescPool);
    updateParameterValues(device, white_mt, col_srmt);
    updateMaterialDescriptorSet(device, white_mt_h, internal_resources,
                                textureSampler);
}
}  // namespace gbg
