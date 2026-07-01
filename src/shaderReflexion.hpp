#include <filesystem>
#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>

#include "Mesh.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"
#include "Scene.hpp"
#include "Shader.hpp"
#include "io_utils/file_utils.hpp"
#include "shaderc/shaderc.hpp"

// setDefaultShader(shader); // sets the default shader (good way to initialize)
// error setShaderCode(shader, filepath, type); // read the file and compile,
// then assign the bytecode if no errors (clear last) (DONE)
// reflectShader(shader); // fill the shader from the bytecodes (clear and
// recreate parameters) (DONE) materialAssignShader(shaderh, scene); // clear
// then create the material parameters from the shader and assign (DONE IN
// MATERIAL)

namespace gbg {

enum ShaderType { VERTEX, FRAGMENT };

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
        if (bind_matparm != NULL) {
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
                    } else if (flags & SPV_REFLECT_TYPE_FLAG_INT) {
                        shader.addParameter(ParameterTypes::INT_PARM);
                    }
                    
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
            for (int i = 0; i < bind_matparm->count; i++)
                shader.addParameter(ParameterTypes::TEXTURE_PARM);
        }
    }
}

inline void reflectShader(Shader& shader) {
    shader.clear();
    if (not shader.getVertShaderCode().empty()) {
        SpvReflectShaderModule vtmod;
        spvReflectCreateShaderModule(
            shader.getVertShaderCode().size() * sizeof(uint32_t),
            shader.getVertShaderCode().data(), &vtmod);
        processShaderModule(vtmod, shader);
    }

    if (not shader.getFragShaderCode().empty()) {
        SpvReflectShaderModule fgmod;
        spvReflectCreateShaderModule(
            shader.getFragShaderCode().size() * sizeof(uint32_t),
            shader.getFragShaderCode().data(), &fgmod);
        processShaderModule(fgmod, shader);
    }
}

inline std::pair<bool, std::string> setShaderCode(gbg::Shader& sh,
                                                  std::filesystem::path path,
                                                  ShaderType type) {
    auto data = readFile(path.string());

    shaderc_shader_kind kind;
    switch (type) {
        case VERTEX:
            kind = shaderc_vertex_shader;
            break;
        case FRAGMENT:
            kind = shaderc_fragment_shader;
            break;
    }

    shaderc::Compiler cmp;
    shaderc::CompilationResult res =
        cmp.CompileGlslToSpv(data.data(), kind, path.filename().c_str());
    if (res.GetCompilationStatus() == shaderc_compilation_status_success) {
        switch (type) {
            case VERTEX:
                sh.setVertShaderCode({res.begin(), res.end()});
                break;
            case FRAGMENT:
                sh.setFragShaderCode({res.begin(), res.end()});
                break;
        }
    }
    return {res.GetCompilationStatus() == shaderc_compilation_status_success,
            res.GetErrorMessage()};
}

}  // namespace gbg
