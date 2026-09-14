#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>

#include "Mesh.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"
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

class _Includer : public shaderc::CompileOptions::IncluderInterface {
   public:
    _Includer(std::vector<std::filesystem::path> paths) : search_paths(paths){};

    struct _IncluderInfo {
        std::string content;
        std::string name;
    };

    shaderc_include_result* GetInclude(const char* requested_source,
                                       shaderc_include_type type,
                                       const char* requesting_source,
                                       size_t include_depth) override {
        std::filesystem::path p = std::filesystem::absolute(requesting_source);
        std::filesystem::path rs = p.parent_path().append(requested_source);
        if (not std::filesystem::exists(rs)) {
            for (auto sp : search_paths) {
                if (std::filesystem::exists(sp.append(requested_source))) {
                    rs = sp;
                    break;
                }
            }
        }
        _IncluderInfo* info = new _IncluderInfo;

        shaderc_include_result* res = new shaderc_include_result{};
        res->user_data = info;

        if (not std::filesystem::exists(rs)) {
            info->content =
                "File not found :: " + std::string(requested_source);
            res->content = info->content.data();
            res->content_length = info->content.length();
            res->source_name_length = 0;
            return res;
        }

        info->content = readFile(rs.native()).data();
        info->name = rs.filename();
        res->content = info->content.data();
        res->content_length = info->content.length();
        res->source_name = info->name.data();
        res->source_name_length = info->name.length();

        return res;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) override {
        delete[] data->content;
        if (data->source_name) delete[] data->source_name;
        delete data;
    }

   private:
    std::vector<std::filesystem::path> search_paths;
};

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

    shaderc::Compiler cmp{};
    shaderc::CompileOptions copt{};
    copt.SetIncluder(std::make_unique<_Includer>({path.parent_path()}));

    shaderc::CompilationResult res =
        cmp.CompileGlslToSpv(data.data(), kind, path.c_str(), copt);
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
