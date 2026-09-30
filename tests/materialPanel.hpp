#pragma once
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>

#include "AppData.hpp"
#include "DependencyTreeFunctions.hpp"
#include "Resource.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "extras/File.hpp"
#include "imgui.h"
#include "io_utils/watcher.hpp"
#include "loaders/texLoader.hpp"
#include "nfd.h"
#include "resourcesUpdate.hpp"
#include "shaderReflexion.hpp"

inline void drawMaterialPanel(AppData& app, gbg::Material& mat) {
    gbg::Scene& sc = app.scene;
    ImGui::PushID(mat.getRID());
    if (ImGui::CollapsingHeader(mat.getName().c_str())) {
        for (auto [num, value] : mat.getValues() | std::views::enumerate) {
            if (const gbg::TextureHandle* h =
                    std::get_if<gbg::TextureHandle>(&value)) {
                gbg::TextureHandle tx_h = *h ? *h : sc.defaults.texture;
                auto& tex = sc.tx_mg.get(tx_h);
                if (ImGui::BeginCombo(
                        ("Texture" + std::to_string(num - 1)).c_str(),
                        tex.getName().c_str())) {
                    // for every texture
                    for (auto& tex2 : sc.tx_mg) {
                        if (ImGui::Selectable(tex2.getName().c_str())) {
                            mat.setParameterValue<gbg::ParameterTypes::TEXTURE>(
                                num, tex2.getHandle());
                            gbg::setDependent(
                                app.dep_tree, mat.representative,
                                gbg::SObjFlags::TEXTURE_PARAMETER_VALUE_M,
                                tex2.representative, gbg::SObjFlags::NEW);
                            app.dep_tree.propagateChange(
                                mat.representative,
                                gbg::SObjFlags::TEXTURE_PARAMETER_VALUE_M);
                        }
                    }

                    ImGui::EndCombo();
                }

            } else if (const glm::vec3* vec = std::get_if<glm::vec3>(&value)) {
                glm::vec3 col = *vec;
                if (ImGui::ColorPicker3(
                        ("Parameter" + std::to_string(num)).c_str(),
                        (float*)&col)) {
                    mat.setParameterValue<gbg::ParameterTypes::VEC3>(num, col);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::PARAMETER_VALUE_M);
                }
            } else if (const glm::vec2* vec = std::get_if<glm::vec2>(&value)) {
                glm::vec2 col = *vec;
                if (ImGui::InputFloat2(
                        ("Parameter" + std::to_string(num)).c_str(),
                        (float*)&col)) {
                    mat.setParameterValue<gbg::ParameterTypes::VEC2>(num, col);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::PARAMETER_VALUE_M);
                }
            } else if (const float* val = std::get_if<float>(&value)) {
                float f = *val;
                if (ImGui::InputFloat(
                        ("Parameter" + std::to_string(num)).c_str(), &f)) {
                    mat.setParameterValue<gbg::ParameterTypes::FLOAT>(num, f);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::PARAMETER_VALUE_M);
                }
            } else if (const int* val = std::get_if<int32_t>(&value)) {
                int i = *val;
                if (ImGui::InputInt(("Parameter" + std::to_string(num)).c_str(),
                                    &i)) {
                    mat.setParameterValue<gbg::ParameterTypes::FLOAT>(num, i);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::PARAMETER_VALUE_M);
                }
            }
        }

        static bool raw = false;

        if (ImGui::Button("New Texture")) {
            ImGui::OpenPopup("New Texture");
            raw = false;
        }

        if (ImGui::BeginPopupModal("New Texture", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            // expand to multiples...
            static char buff[1024] = "";
            static char name[64] = "";
            if (ImGui::Button("Search")) {
                nfdu8char_t* outpath = nullptr;
                nfdopendialognargs_t args{};
                std::filesystem::path path(buff);
                std::filesystem::path root_dir = path.parent_path();
                args.defaultPath = root_dir.c_str();
                nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
                if (res == NFD_OKAY) {
                    if (strlen(outpath) < sizeof(buff))
                        std::strcpy(buff, outpath);
                    path = buff;
                    std::strcpy(name, path.filename().c_str());
                    NFD_FreePathU8(outpath);
                }
            }

            ImGui::InputText("File path", buff, sizeof(buff));
            ImGui::InputText("Name", name, sizeof(name));
            ImGui::Checkbox("Raw", &raw);

            if (ImGui::Button("Confirm")) {
                auto& new_tx = sc.tx_mg.create(name);
                gbg::createRepresentative(app.dep_tree, new_tx,
                                          gbg::ResourceTypes::TEXTURE,
                                          gbg::SObjFlags::NEW);
                loadTexture(buff, new_tx);
                new_tx.raw = raw;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    ImGui::PopID();
}

inline void drawShaderPannel(AppData& app) {
    gbg::Scene& sc = app.scene;
    nfdu8char_t* outpath = nullptr;
    static char buff[1024] = "";
    static std::filesystem::path chosen_path;

    for (auto& shader : sc.sh_mg) {
        if (ImGui::CollapsingHeader(shader.getName().c_str())) {
            for (auto [num, parm] :
                 shader.getParameters() | std::views::enumerate) {
                ImGui::Text("Position: %ld, Type: %s", num,
                            gbg::parmTypeToString[to_underlying(parm)].data());
            }
        }
    }

    if (ImGui::Button("New Shader")) {
        nfdopendialognargs_t args = {0};
        nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
        if (res == NFD_OKAY) {
            if (strlen(outpath) < sizeof(buff)) strcpy(buff, outpath);
            NFD_FreePathU8(outpath);
            ImGui::OpenPopup("Load Shader");
        }
    }

    if (ImGui::BeginPopupModal("Load Shader", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        auto pt = std::filesystem::path(buff);
        std::string name = pt.filename().replace_extension();
        pt = pt.parent_path();
        std::vector<std::filesystem::path> paths;
        for (const auto& pt : std::filesystem::directory_iterator{pt}) {
            if (pt.path().filename().replace_extension() == name) {
                paths.push_back(pt.path());
            }
        }

        if (ImGui::BeginListBox("Detected Shader Files")) {
            for (const auto& path : paths) {
                std::string s = path.string();
                if (s.size() > 20) {
                    s = "..." + s.substr(s.size() - 20);
                }
                ImGui::Selectable(s.c_str(), false);
            }
            ImGui::EndListBox();
        }

        if (ImGui::Button("Load")) {
            // Shader Creation
            gbg::Shader& sh = sc.sh_mg.create(name);

            gbg::createRepresentative(app.dep_tree, sh,
                                      gbg::ResourceTypes::SHADER,
                                      gbg::SObjFlags::NEW);

            for (auto pt : paths) {
                auto res =
                    app.file_w.createFile(pt.native())
                        .and_then([&](gbg::File* file) {
                            return gbg::setGlslShaderCode(sh, {file->path});
                        });
                if (res) {
                    std::cout << res.value() << std::endl;
                    sh.setCode(sc.getDefaultShader().getCode(
                                   gbg::ShaderTypes::FRAGMENT),
                               sc.getDefaultShader().getCodeFile(
                                   gbg::ShaderTypes::FRAGMENT),
                               gbg::ShaderTypes::FRAGMENT);
                    sh.setCode(sc.getDefaultShader().getCode(
                                   gbg::ShaderTypes::VERTEX),
                               sc.getDefaultShader().getCodeFile(
                                   gbg::ShaderTypes::VERTEX),
                               gbg::ShaderTypes::VERTEX);
                }
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void drawNewMaterial(AppData& app) {
    gbg::Scene& sc = app.scene;
    if (ImGui::Button("New Material")) {
        ImGui::OpenPopup("New Material");
    }

    static gbg::ShaderHandle selected = gbg::ShaderHandle();

    if (ImGui::BeginPopupModal("New Material", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        const char* def = selected ? sc.sh_mg.get(selected).getName().c_str()
                                   : "pick - shader";
        if (ImGui::BeginCombo("Pick Shader", def)) {
            for (auto& sh : sc.sh_mg) {
                if (ImGui::Selectable(sh.getName().c_str())) {
                    selected = sh.getHandle();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Create")) {
            if (selected) {
                auto& sh = sc.sh_mg.get(selected);
                auto& mt = sc.mat_mg.create(
                    "Material" + std::to_string(sc.mat_mg.nextIndex()));
                gbg::createRepresentative(
                    app.dep_tree, mt, gbg::ResourceTypes::MATERIAL,
                    gbg::SObjFlags::NEW |
                        gbg::SObjFlags::PARAMETER_INTERFACE_M);
                mt.setShader(selected);
                gbg::setDependent(app.dep_tree, mt.representative,
                                  gbg::SObjFlags::PARAMETER_INTERFACE_M,
                                  sh.representative,
                                  gbg::SObjFlags::CODE_M | gbg::SObjFlags::NEW);

                ImGui::CloseCurrentPopup();
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.f, 1.f),
                                   "You must select a shader");
            }
        }

        ImGui::EndPopup();
    }
};
