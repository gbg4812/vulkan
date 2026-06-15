#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>

#include "Mesh.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"
#include "Scene.hpp"
#include "Shader.hpp"

namespace gbg {

inline void processShaderModule(const SpvReflectShaderModule& shmod,
                                Shader& shader) {
    if (shmod.shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
        uint32_t count;
        spvReflectEnumerateInputVariables(&shmod, &count, nullptr);
        std::vector<SpvReflectInterfaceVariable*> ivars(count);
        if (spvReflectEnumerateInputVariables(&shmod, &count, ivars.data()) !=
            SPV_REFLECT_RESULT_SUCCESS) {
            throw std::runtime_error("Filed to get input variables");
        }

        std::map<SpvReflectFormat, AttributeTypes> spv_to_attr = {
            {SPV_REFLECT_FORMAT_R32_SFLOAT, AttributeTypes::FLOAT_ATTR},
            {SPV_REFLECT_FORMAT_R32G32_SFLOAT, AttributeTypes::VEC2_ATTR},
            {SPV_REFLECT_FORMAT_R32G32B32_SFLOAT, AttributeTypes::VEC3_ATTR}};

        for (SpvReflectInterfaceVariable* ivar_p : ivars) {
            shader.addAttribute(ivar_p->location, spv_to_attr[ivar_p->format]);
        }
    }

    auto nontex =
        std::views::filter([](ParameterTypes p) { return p != TEXTURE_PARM; });
    auto tex =
        std::views::filter([](ParameterTypes p) { return p == TEXTURE_PARM; });

    if ((shader.getParameters() | nontex).empty()) {
        SpvReflectResult res;
        const SpvReflectDescriptorBinding* bind_matparm =
            spvReflectGetDescriptorBinding(&shmod, 0, 1, &res);
        std::span<SpvReflectBlockVariable> variables(
            bind_matparm->block.members, bind_matparm->block.member_count);

        for (SpvReflectBlockVariable& var : variables) {
            SpvReflectTypeFlags flags = var.type_description->type_flags;
            if (flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                    int comps = var.numeric.vector.component_count;
                    switch (comps) {
                        case 2:
                            shader.addParameter(ParameterTypes::VEC2_PARM);
                            break;
                        case 3:
                            shader.addParameter(ParameterTypes::VEC3_PARM);
                            break;
                    }
                }
            } else {
                if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                    shader.addParameter(ParameterTypes::FLOAT_PARM);
                }
            }
        }
    }  // process non texture parms

    {
        SpvReflectResult res;
        const SpvReflectDescriptorBinding* bind_matparm =
            spvReflectGetDescriptorBinding(&shmod, 1, 1, &res);

        if (res == SPV_REFLECT_RESULT_SUCCESS and
            bind_matparm->descriptor_type ==
                SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
            shader.addParameter(ParameterTypes::TEXTURE_PARM);
        }
    }
}

inline void initShader(ShaderHandle shh, Scene& scene) {
    auto& shader = scene.sh_mg.get(shh);
    SpvReflectShaderModule vtmod;
    spvReflectCreateShaderModule(shader.getVertShaderCode().size(),
                                 shader.getVertShaderCode().data(), &vtmod);

    SpvReflectShaderModule fgmod;
    spvReflectCreateShaderModule(shader.getFragShaderCode().size(),
                                 shader.getFragShaderCode().data(), &fgmod);

    processShaderModule(vtmod, shader);
    processShaderModule(fgmod, shader);
}

}  // namespace gbg
