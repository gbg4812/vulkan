#include "shaderUtils.hpp"

#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

#include <cstring>
#include <expected>
#include <print>
#include <ranges>
#include <type_traits>

#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "build_constants.hpp"
#include "extras/File.hpp"
#include "traits/traits.hpp"
#include "gbg_traits.hpp"

static Slang::ComPtr<slang::IGlobalSession> globalSession;

namespace gbg {
void initShaderCompiler() {
    slang::createGlobalSession(globalSession.writeRef());
}

Slang::ComPtr<slang::ISession> createSession(const char* shader_dir) {
    slang::SessionDesc desc = {};

    Slang::ComPtr<slang::ISession> spirvSession;
    std::array<slang::CompilerOptionEntry, 3> entries{};
    entries[0].name = slang::CompilerOptionName::VulkanUseEntryPointName;
    entries[0].value = {slang::CompilerOptionValueKind::Int, 1, 0, nullptr,
                        nullptr};
    entries[1].name = slang::CompilerOptionName::EmitSpirvDirectly;
    entries[1].value = {slang::CompilerOptionValueKind::Int, 1, 0, nullptr,
                        nullptr};
    entries[2].name = slang::CompilerOptionName::MatrixLayoutColumn;
    entries[2].value = {slang::CompilerOptionValueKind::Int, 1, 0, nullptr,
                        nullptr};

    desc.compilerOptionEntries = entries.data();
    desc.compilerOptionEntryCount = entries.size();

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv_1_6");

    desc.targets = &targetDesc;
    desc.targetCount = 1;

    std::array<const char*, 2> searchPaths = {CONTENTS_DIR "shaders/",
                                              shader_dir};
    desc.searchPaths = searchPaths.data();
    desc.searchPathCount = searchPaths.size();

    globalSession->createSession(desc, spirvSession.writeRef());
    return spirvSession;
}

void reflectParameters(Shader& shader,
                       slang::TypeLayoutReflection* scopeTypeLayout) {
    switch (scopeTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Struct: {
            int paramCount = scopeTypeLayout->getFieldCount();
            for (int i = 0; i < paramCount; i++) {
                slang::VariableLayoutReflection* fieldVarLayout =
                    scopeTypeLayout->getFieldByIndex(i);
                if (std::strcmp(fieldVarLayout->getName(), "mat") == 0) {
                    auto matTypeLayout =
                        fieldVarLayout->getTypeLayout()->getElementTypeLayout();

                    for (int j = 0; j < matTypeLayout->getFieldCount(); j++) {
                        slang::VariableLayoutReflection* matFieldLayout =
                            matTypeLayout->getFieldByIndex(j);
                        auto matFieldTypeLayout =
                            matFieldLayout->getTypeLayout();
                        ISlangBlob* blob;
                        matFieldLayout->getVariable()->getDefaultValueBlob(
                            &blob);
                        std::string name = matFieldLayout->getName();
                        gbg::parm_vt def;
                        switch (matFieldTypeLayout->getKind()) {
                            case slang::TypeReflection::Kind::Vector: {
                                switch (matFieldTypeLayout->getColumnCount()) {
                                    case 2: {
                                        def = vec2_t();
                                    } break;
                                    case 3: {
                                        def = vec3_t();
                                    } break;
                                }
                            } break;
                            case slang::TypeReflection::Kind::Scalar: {
                                switch (matFieldTypeLayout->getScalarType()) {
                                    case slang::TypeReflection::Int32: {
                                        def = 0;
                                    } break;
                                    case slang::TypeReflection::Float32: {
                                        def = 0.0f;
                                    } break;
                                    case slang::TypeReflection::Bool: {
                                        def = true;
                                    } break;
                                }
                            } break;
                            case slang::TypeReflection::Kind::Resource: {
                                if ((matFieldTypeLayout->getResourceShape() &
                                     SLANG_RESOURCE_BASE_SHAPE_MASK) ==
                                    SLANG_TEXTURE_2D) {
                                    def = TextureHandle();
                                }
                            } break;
                        }

                        if(blob) {
                            std::visit(overloads{
                                [&](auto& val) {
                                    val = *reinterpret_cast<const std::remove_reference_t<decltype(val)>*>(blob->getBufferPointer());
                                }
                            }, def);
                        }
                        shader.addParameter(def, name);
                    }
                }
            }

        } break;
    }
}

void reflectAttributes(Shader& shader, slang::EntryPointReflection* entry_pt) {
    auto entryPtLayout = entry_pt->getVarLayout();
    std::println("{}", to_underlying(entry_pt->getStage()));
    auto entryPtTypeLayout = entryPtLayout->getTypeLayout();
    switch (entryPtTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Struct:
            break;
        case slang::TypeReflection::Kind::ConstantBuffer:
            entryPtTypeLayout = entryPtTypeLayout->getElementTypeLayout();
            break;
    }

    int parmCount = entryPtTypeLayout->getFieldCount();
    slang::TypeLayoutReflection* vertexInputReflection = nullptr;
    for (int i : std::views::iota(0, parmCount)) {
        auto fieldVarLayout = entryPtTypeLayout->getFieldByIndex(i);
        // TODO: do I want it so rigid?
        if (std::strcmp("vs_in", fieldVarLayout->getName()) == 0) {
            vertexInputReflection = fieldVarLayout->getTypeLayout();
        }
    }

    if (vertexInputReflection) {
        auto attribCount = vertexInputReflection->getFieldCount();
        for (auto i : std::views::iota(0u, attribCount)) {
            auto attribVar = vertexInputReflection->getFieldByIndex(i);
            auto attribTypeLayout = attribVar->getTypeLayout();
            switch (attribTypeLayout->getKind()) {
                case slang::TypeReflection::Kind::Vector: {
                    switch (attribTypeLayout->getColumnCount()) {
                        case 2:
                            shader.addAttribute(i, AttributeTypes::VEC2_ATTR);
                            break;
                        case 3:
                            shader.addAttribute(i, AttributeTypes::VEC3_ATTR);
                            break;
                    }
                } break;
                case slang::TypeReflection::Kind::Scalar: {
                    switch (attribTypeLayout->getScalarType()) {
                        case slang::TypeReflection::Float32: {
                            shader.addAttribute(i, AttributeTypes::FLOAT_ATTR);

                        } break;
                    }
                } break;
            }
        }
    }
}

// if it gets too slow try precompiling modules
std::expected<std::string, std::string> compileShader(FileManager& f_m,
                                                      Shader& shader) {
    if (not globalSession)
        return std::unexpected("You need to call init shader compiler first!");

    // TODO: get all code files
    auto& code_file = f_m.get(shader.getCodeFile(0));

    auto pr_path = code_file.path.parent_path();
    auto module_name = code_file.path.filename().replace_extension();

    Slang::ComPtr<slang::ISession> session = createSession(pr_path.c_str());

    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    Slang::ComPtr<slang::IModule> module;
    module =
        session->loadModule(module_name.c_str(), diagnosticsBlob.writeRef());

    std::string diagnostics =
        diagnosticsBlob ? (const char*)diagnosticsBlob->getBufferPointer() : "";
    if (!module) return std::unexpected(diagnostics);

    auto entryPtCount = module->getDefinedEntryPointCount();
    std::vector<slang::IComponentType*> components(entryPtCount);
    for (int i : std::views::iota(0, entryPtCount)) {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        module->getDefinedEntryPoint(i, (slang::IEntryPoint**)&components[i]);
    }
    components.push_back(module);

    Slang::ComPtr<slang::IComponentType> program;
    SlangResult res = session->createCompositeComponentType(
        components.data(), components.size(), program.writeRef(),
        diagnosticsBlob.writeRef());

    diagnostics +=
        diagnosticsBlob ? (const char*)diagnosticsBlob->getBufferPointer() : "";
    if (SLANG_FAILED(res)) {
        return std::unexpected(diagnostics);
    }

    Slang::ComPtr<slang::IComponentType> linkedComponent;
    res = program->link(linkedComponent.writeRef(), diagnosticsBlob.writeRef());

    diagnostics +=
        diagnosticsBlob ? (const char*)diagnosticsBlob->getBufferPointer() : "";
    if (SLANG_FAILED(res)) {
        return std::unexpected(diagnostics);
    }

    Slang::ComPtr<slang::IBlob> spirvCode;
    res = linkedComponent->getTargetCode(0, spirvCode.writeRef(),
                                         diagnosticsBlob.writeRef());

    diagnostics +=
        diagnosticsBlob ? (const char*)diagnosticsBlob->getBufferPointer() : "";
    if (SLANG_FAILED(res)) {
        return std::unexpected(diagnostics);
    }

    std::vector<uint32_t> spirvCodeI32(spirvCode->getBufferSize() /
                                       sizeof(uint32_t));
    std::memcpy(spirvCodeI32.data(), spirvCode->getBufferPointer(),
                spirvCode->getBufferSize());
    shader.setByteCode(0, spirvCodeI32);

    // clear parameters and attributes
    shader.clear();
    slang::ProgramLayout* layout = linkedComponent->getLayout();
    auto scopeTypeLayout = layout->getGlobalParamsTypeLayout();

    slang::EntryPointReflection* vt_entry_pt =
        layout->findEntryPointByName("mainVertex");
    if (vt_entry_pt) {
        reflectAttributes(shader, vt_entry_pt);
        shader.setCodeTypeFlags(0, ShaderTypeFlags::SHADER_TYPE_VERTEX);
    }

    slang::EntryPointReflection* fg_entry_pt =
        layout->findEntryPointByName("mainPixel");
    if (fg_entry_pt) {
        reflectParameters(shader, scopeTypeLayout);
        shader.setCodeTypeFlags(0, ShaderTypeFlags::SHADER_TYPE_FRAGMENT);
    }

    return diagnostics;
}

std::expected<void, std::string> checkShaderCompatibility(
    const Mesh& mesh, const Shader& shader) {
    return {};
}

}  // namespace gbg
