#pragma once
#include <vulkan/vulkan_core.h>

#include <expected>
#include <stdexcept>
#include <unordered_map>

#include "InternalSceneData.hpp"
#include "MaterialFunctions.hpp"
#include "PerObjectPushConstant.hpp"
#include "Shader.hpp"
#include "build_constants.hpp"
#include "loaders/objLoader.hpp"
#include "resourcesUpdate.hpp"
#include "shaderUtils.hpp"
#include "srMaterial.hpp"
#include "vk_utils/vkRenderPass.hpp"

namespace gbg {
inline void loadRendererResources(
    vkDevice device, VkDescriptorSetLayout globalDescSet,
    InternalSceneData& internal_resources,
    const std::unordered_map<std::string, vkRenderPass>& renderPasses,
    VkDescriptorPool materialDescPool, VkSampler textureSampler) {
    auto sc = internal_resources.scene;

    auto& ms_mg = sc->getMeshManager();
    auto& mt_mg = sc->getMaterialManager();
    auto& sh_mg = sc->getShaderManager();

    auto& col_sh = sh_mg.create("PlainColorShader");
    auto& col_sh_f = internal_resources.file_m->create(
        CONTENTS_DIR "/shaders/slang/plain_color.slang");

    col_sh.topology = PrimitiveInterpretation::LINES;
    col_sh.appendCodeFile(col_sh_f.getHandle());
    col_sh.casts_shadow = false;

    auto res = compileShader(*internal_resources.file_m, col_sh);
    if (not res) {
        std::cout << res.error() << std::endl;
    }

    auto& white_mt = mt_mg.create("White Material");
    white_mt.setShader(col_sh.getHandle());
    setParametersFromShader(col_sh, white_mt);
    white_mt.setParameterValue<ParameterTypes::VEC3>(
        0, glm::vec3(1.0f, 1.0f, 1.0f));

    sc->defaults.material = white_mt.getHandle();
    if (objLoadMeshes(CONTENTS_DIR "models/RendererObjects.obj", sc.get(),
                      sc->root, {})
            .empty()) {
        throw std::runtime_error("Failed to load renderer objects");
    }

    for (auto& msh : ms_mg) {
        createMeshVkResources(device, msh.getHandle(), internal_resources);
    }

    VkPushConstantRange pushConstants{};
    pushConstants.offset = 0;
    pushConstants.size = sizeof(PerObjectPushConstant);
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    auto& col_srsh = internal_resources.srsh_mg.create("srColorShader");
    createShaderVkResources(device, col_sh, col_srsh, renderPasses.at("color"),
                            {globalDescSet}, {pushConstants});

    auto& col_srmt = internal_resources.srmat_mg.create("srWhiteMaterial");
    createMaterialVkResources(device, white_mt.getHandle(), internal_resources,
                              materialDescPool);
    updateParameterValues(device, white_mt, col_srmt);
    updateMaterialDescriptorSet(device, white_mt.getHandle(),
                                internal_resources, textureSampler);
}
}  // namespace gbg
