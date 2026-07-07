#pragma once
#include <unordered_map>
#include "InternalSceneData.hpp"
#include "shaderReflexion.hpp"
#include "resourcesUpdate.hpp"
#include "vk_utils/vkRenderPass.hpp"
#include "loaders/objLoader.hpp"

namespace gbg  {
    inline void loadRendererResources(vkDevice device, VkDescriptorSetLayout globalDescSet,
        InternalSceneData& internal_resources,
        const std::unordered_map<std::string, vkRenderPass>& renderPasses, VkDescriptorPool materialDescPool, VkSampler textureSampler) {

        Scene* internal_scene = internal_resources.scene;
        CREATE_AND_GET(color_shader, internal_scene->sh_mg, "PlainColorShader");
        setShaderCode(color_shader, "data/models/RendererResources/plain_color.vert", VERTEX);
        setShaderCode(color_shader, "data/models/RendererResources/plain_color.frag", FRAGMENT);
        reflectShader(color_shader);
        color_shader.topology = LINES;

        CREATE_AND_GET(white_material, internal_scene->mat_mg, "WhiteMaterial");
        white_material.setShader(color_shader_h, color_shader);
        white_material.setParameterValue<VEC3_PARM>(0, glm::vec3(1.0f, 1.0f, 1.0f));

        objLoader("data/models/RendererResources/RendererObjects.obj", internal_scene ,internal_scene->root, white_material_h);

        auto& ms_mg = internal_resources.scene->getMeshManager();
        auto& mt_mg = internal_resources.scene->getMaterialManager();
        auto& sh_mg = internal_resources.scene->getShaderManager();


        for(auto msh_h : internal_scene->ms_mg) {
            updateMesh(device, msh_h, internal_resources);
        }

        color_shader.shadow = false;

        updateShader(device, color_shader_h, internal_resources, renderPasses.at("color"), {globalDescSet});

        updateMaterial(device, white_material_h, internal_resources, materialDescPool, textureSampler);
    }
}
