#include "shaderUtils.hpp"

#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

#include <cstring>
#include <expected>
#include <print>

#include "Shader.hpp"
#include "Texture.hpp"
#include "build_constants.hpp"
#include "extras/File.hpp"

static Slang::ComPtr<slang::IGlobalSession> globalSession;

namespace gbg {
void initShaderCompiler() {
    slang::createGlobalSession(globalSession.writeRef());
}

Slang::ComPtr<slang::ISession> createSession(const char* shader_dir) {
    slang::SessionDesc desc = {};

    Slang::ComPtr<slang::ISession> spirvSession;
    std::array<slang::CompilerOptionEntry, 2> entries{};
    entries[0].name = slang::CompilerOptionName::VulkanUseEntryPointName;
    entries[0].value = {slang::CompilerOptionValueKind::Int, 1, 0, nullptr,
                        nullptr};
    entries[1].name = slang::CompilerOptionName::EmitSpirvDirectly;
    entries[1].value = {slang::CompilerOptionValueKind::Int, 1, 0, nullptr,
                        nullptr};

    desc.compilerOptionEntries = entries.data();
    desc.compilerOptionEntryCount = entries.size();

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv_1_6");

    desc.targets = &targetDesc;
    desc.targetCount = 1;

    std::array<const char*, 2> searchPaths = {CONTENTS_DIR "shaders/slang/",
                                              shader_dir};
    desc.searchPaths = searchPaths.data();
    desc.searchPathCount = searchPaths.size();

    globalSession->createSession(desc, spirvSession.writeRef());
    return spirvSession;
}

// if it gets too slow try precompiling modules
std::expected<std::string, std::string> compileShader(FileManager& f_m,
                                                      Shader& shader) {
    if (not globalSession)
        return std::unexpected("You need to call init shader compiler first!");

    ShaderTypes type =ShaderTypes::COMBINED;
    if(shader.getCodeFile(ShaderTypes::COMBINED).empty()) {
        type = ShaderTypes::VERTEX;
    }
    auto& code_file = f_m.get(shader.getCodeFile(type));

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

    Slang::ComPtr<slang::IComponentType> linkedComponent;
    SlangResult res =
        module->link(linkedComponent.writeRef(), diagnosticsBlob.writeRef());

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

    std::vector<uint32_t> spirvCodeI32(spirvCode->getBufferSize() *
                                       sizeof(uint32_t));
    std::memcpy(spirvCodeI32.data(), spirvCode->getBufferPointer(),
                spirvCode->getBufferSize());
    shader.setByteCode(spirvCodeI32, type);

    // clear parameters and attributes
    shader.clear();
    slang::ProgramLayout* layout = module->getLayout();

    auto scopeVarLayout = layout->getGlobalParamsVarLayout();
    auto scopeTypeLayout = scopeVarLayout->getTypeLayout();

    switch (scopeTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Struct: {
            std::println("parameters: ");
            int paramCount = scopeTypeLayout->getFieldCount();
            for (int i = 0; i < paramCount; i++) {
                slang::VariableLayoutReflection* fieldVarLayout =
                    scopeTypeLayout->getFieldByIndex(i);
                std::println("{}", fieldVarLayout->getName());
                if (std::strcmp(fieldVarLayout->getName(), "mat") == 0) {
                    auto matTypeLayout =
                        fieldVarLayout->getTypeLayout()->getElementTypeLayout();
                    std::println("{}", matTypeLayout->getType()->getName());

                    for (int j = 0; j < matTypeLayout->getFieldCount(); j++) {
                        slang::VariableLayoutReflection* matFieldLayout =
                            matTypeLayout->getFieldByIndex(j);
                        auto matFieldTypeLayout =
                            matFieldLayout->getTypeLayout();
                        std::println("{}", matFieldLayout->getName());
                        ISlangBlob* blob;
                        matFieldLayout->getVariable()->getDefaultValueBlob(
                            &blob);
                        std::string name = matFieldLayout->getName();
                        switch (matFieldTypeLayout->getKind()) {
                            case slang::TypeReflection::Kind::Vector: {
                                switch (matFieldTypeLayout->getColumnCount()) {
                                    case 2:
                                        shader.addParameter(
                                            *reinterpret_cast<const vec2_t*>(
                                                blob->getBufferPointer()), name);
                                        break;
                                    case 3:
                                        shader.addParameter(
                                            *reinterpret_cast<const vec3_t*>(
                                                blob->getBufferPointer()), name);
                                        break;
                                }
                            } break;
                            case slang::TypeReflection::Kind::Scalar: {
                                switch (matFieldTypeLayout->getScalarType()) {
                                    case slang::TypeReflection::Int32: {
                                        shader.addParameter(
                                            *reinterpret_cast<const int*>(
                                                blob->getBufferPointer()), name);
                                    } break;
                                    case slang::TypeReflection::Float32: {
                                        shader.addParameter(
                                            *reinterpret_cast<const float*>(
                                                blob->getBufferPointer()), name);

                                    } break;
                                }
                            } break;
                            case slang::TypeReflection::Kind::Resource: {
                                if ((matFieldTypeLayout->getResourceShape() &
                                     SLANG_RESOURCE_BASE_SHAPE_MASK) ==
                                    SLANG_TEXTURE_2D) {
                                        shader.addParameter(TextureHandle());
                                }
                            } break;
                        }
                    }
                }
            }

        } break;
    }

    return diagnostics;
}

std::expected<void, std::string> checkShaderCompatibility(
    const Mesh& mesh, const Shader& shader) {
    return {};
}

}  // namespace gbg
