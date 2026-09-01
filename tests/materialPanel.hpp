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
                    for (auto texh : sc.tx_mg) {
                        auto& tex2 = sc.tx_mg.get(texh);
                        if (ImGui::Selectable(tex2.getName().c_str())) {
                            mat.setParameterValue<
                                gbg::ParameterTypes::TEXTURE_PARM>(num, texh);
                            gbg::setDependent(app.dep_tree, mat,
                                              gbg::SObjFlags::TEXTURE_CHANGED,
                                              tex2, gbg::SObjFlags::NEW);
                            app.dep_tree.propagateChange(
                                mat.representative,
                                gbg::SObjFlags::TEXTURE_CHANGED);
                        }
                    }

                    ImGui::EndCombo();
                }

            } else if (const glm::vec3* vec = std::get_if<glm::vec3>(&value)) {
                glm::vec3 col = *vec;
                if (ImGui::ColorPicker3(
                        ("Parameter" + std::to_string(num)).c_str(),
                        (float*)&col)) {
                    mat.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(num,
                                                                          col);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::DIRTY_PARAMETER);
                }
            } else if (const glm::vec2* vec = std::get_if<glm::vec2>(&value)) {
                glm::vec2 col = *vec;
                if (ImGui::InputFloat2(
                        ("Parameter" + std::to_string(num)).c_str(),
                        (float*)&col)) {
                    mat.setParameterValue<gbg::ParameterTypes::VEC2_PARM>(num,
                                                                          col);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::DIRTY_PARAMETER);
                }
            } else if (const float* val = std::get_if<float>(&value)) {
                float f = *val;
                if (ImGui::InputFloat(
                        ("Parameter" + std::to_string(num)).c_str(), &f)) {
                    mat.setParameterValue<gbg::ParameterTypes::FLOAT_PARM>(num,
                                                                           f);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::DIRTY_PARAMETER);
                }
            } else if (const int* val = std::get_if<int32_t>(&value)) {
                int i = *val;
                if (ImGui::InputInt(("Parameter" + std::to_string(num)).c_str(),
                                    &i)) {
                    mat.setParameterValue<gbg::ParameterTypes::FLOAT_PARM>(num,
                                                                           i);
                    app.dep_tree.propagateChange(
                        mat.representative, gbg::SObjFlags::DIRTY_PARAMETER);
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
                auto hand = sc.tx_mg.create(name);
                gbg::createRepresentative(app.dep_tree, hand, sc.tx_mg,
                                          gbg::ResourceTypes::TEXTURE,
                                          gbg::SObjFlags::NEW);
                loadTexture(buff, &sc, hand);
                sc.tx_mg.get(hand).raw = raw;
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

    for (auto shh : sc.sh_mg) {
        auto& shader = sc.sh_mg.get(shh);
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
            ImGui::OpenPopup("New Shader");
        }
    }

    if (ImGui::BeginPopupModal("Load Shader", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        auto pt = std::filesystem::path(buff);
        std::string name = pt.filename().replace_extension();
        pt = pt.parent_path();
        std::list<std::filesystem::path> paths;
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
            gbg::ShaderHandle shh = sc.sh_mg.create(name);
            gbg::Shader& sh = sc.sh_mg.get(shh);

            gbg::createRepresentative(app.dep_tree, shh, sc.sh_mg,
                                      gbg::ResourceTypes::SHADER,
                                      gbg::SObjFlags::NEW);

            auto vert = [](const std::filesystem::path& path) {
                return path.extension() == ".vert";
            };
            auto frag = [](const std::filesystem::path& path) {
                return path.extension() == ".frag";
            };

            // Continue TODO(GUILLEM):
            auto res = gbg::setShaderCode(
                sh, *(std::ranges::find_if(paths, vert)), gbg::VERTEX);
            if (not res.first) {
                std::cout << res.second << std::endl;
                sh.setVertShaderCode(
                    sc.sh_mg.get(sc.defaults.shader).getVertShaderCode());
            }
            res = gbg::setShaderCode(sh, *std::ranges::find_if(paths, frag),
                                     gbg::FRAGMENT);
            if (not res.first) {
                std::cout << res.second << std::endl;
                sh.setVertShaderCode(
                    sc.sh_mg.get(sc.defaults.shader).getFragShaderCode());
            }

            watch({std::ranges::find_if(paths, vert)->string(),
                   std::ranges::find_if(paths, frag)->string()},
                  (uint32_t)WatchEvents::MODFY, [&]() {
                      auto res = gbg::setShaderCode(
                          sh, *std::ranges::find_if(paths, vert), gbg::VERTEX);
                      if (not res.first) {
                          std::cout << res.second << std::endl;
                      } else {
                          std::cout << "Shader recompiled successfuly"
                                    << std::endl;
                      }
                      res = gbg::setShaderCode(
                          sh, *std::ranges::find_if(paths, frag),
                          gbg::FRAGMENT);
                      if (not res.first) {
                          std::cout << res.second << std::endl;
                      } else {
                          std::cout << "Shader recompiled successfuly"
                                    << std::endl;
                      }

                      app.dep_tree.propagateChange(
                          sh.representative, gbg::SObjFlags::DIRTY_SHADER_CODE);
                  });
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
            for (auto shh : sc.sh_mg) {
                auto& sh = sc.sh_mg.get(shh);
                if (ImGui::Selectable(sh.getName().c_str())) {
                    selected = shh;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Create")) {
            if (selected) {
                auto& sh = sc.sh_mg.get(selected);
                auto mh = sc.mat_mg.create("Material" +
                                           std::to_string(sc.mat_mg.nextID()));
                gbg::createRepresentative(
                    app.dep_tree, mh, sc.mat_mg, gbg::ResourceTypes::MATERIAL,
                    gbg::SObjFlags::NEW | gbg::SObjFlags::SHADER_CHANGED);
                auto& mt = sc.mat_mg.get(mh);
                mt.setShader(selected);
                gbg::setDependent(
                    app.dep_tree, mt, gbg::SObjFlags::SHADER_CHANGED, sh,
                    gbg::SObjFlags::DIRTY_SHADER_CODE | gbg::SObjFlags::NEW);

                ImGui::CloseCurrentPopup();
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.f, 1.f),
                                   "You must select a shader");
            }
        }

        ImGui::EndPopup();
    }
};
