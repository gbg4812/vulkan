#pragma once
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Scene.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "imgui.h"
#include "io_utils/watcher.hpp"
#include "loaders/texLoader.hpp"
#include "nfd.h"
#include "shaderReflexion.hpp"

inline void drawMaterialPanel(gbg::Scene& sc, gbg::Material& mat) {
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
                            mat.setFlags(gbg::ResourceFlags::DIRTY);
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
                    mat.setFlags(gbg::ResourceFlags::DIRTY);
                }
            } else if (const glm::vec2* vec = std::get_if<glm::vec2>(&value)) {
                glm::vec2 col = *vec;
                if (ImGui::InputFloat2(
                        ("Parameter" + std::to_string(num)).c_str(),
                        (float*)&col)) {
                    mat.setParameterValue<gbg::ParameterTypes::VEC2_PARM>(num,
                                                                          col);
                    mat.setFlags(gbg::ResourceFlags::DIRTY);
                }
            } else if (const float* val = std::get_if<float>(&value)) {
                float f = *val;
                if (ImGui::InputFloat(
                        ("Parameter" + std::to_string(num)).c_str(), &f)) {
                    mat.setParameterValue<gbg::ParameterTypes::FLOAT_PARM>(num,
                                                                           f);
                    mat.setFlags(gbg::ResourceFlags::DIRTY);
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
            nfdu8char_t* outpath = nullptr;
            static std::filesystem::path pt = std::filesystem::current_path();
            static char name[64] = "";
            if (ImGui::Button("Search")) {
                static char buff[1024] = "";
                nfdopendialognargs_t args = {0};
                args.defaultPath = pt.remove_filename().c_str();
                nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
                if (res == NFD_OKAY) {
                    if (strlen(outpath) < sizeof(buff)) strcpy(buff, outpath);
                    NFD_FreePathU8(outpath);
                    pt = buff;
                }
            }

            pt.filename().replace_extension().string().copy(name, sizeof(name));
            
            static char buff[1024] = "";
            pt.string().copy(buff, sizeof(buff));
            
            ImGui::InputText("File path", buff, sizeof(buff));
            ImGui::InputText("Name", name, sizeof(name));
            ImGui::Checkbox("Raw", &raw);

            if (ImGui::Button("Confirm")) {
                auto hand = sc.tx_mg.create(name);
                loadTexture(buff, &sc, hand);
                sc.tx_mg.get(hand).raw = raw;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    ImGui::PopID();
}

inline void drawShaderPannel(gbg::Scene& sc) {
    nfdu8char_t* outpath = nullptr;
    static char buff[1024] = "";
    static std::filesystem::path chosen_path;

    for (auto shh : sc.sh_mg) {
        auto& shader = sc.sh_mg.get(shh);
        if (ImGui::CollapsingHeader(shader.getName().c_str())) {
            for (auto [num, parm] :
                 shader.getParameters() | std::views::enumerate) {
                ImGui::Text("Position: %ld, Type: %s", num,
                            gbg::parmTypeToString[parm].data());
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
    if (ImGui::BeginPopupModal("New Shader", NULL,
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
            gbg::ShaderHandle shh = sc.sh_mg.create(name);
            gbg::Shader& sh = sc.sh_mg.get(shh);
            // Shader Creation

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
            gbg::reflectShader(sh);

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

                      gbg::reflectShader(sh);

                      for (gbg::MaterialHandle mh : sc.mat_mg) {
                          gbg::Material& mat = sc.mat_mg.get(mh);
                          if (mat.getShaderHandle() == shh) {
                              mat.setShader(shh, sh);
                              mat.setFlags(gbg::ResourceFlags::DIRTY);
                          }
                      }
                      sh.setFlags(gbg::ResourceFlags::DIRTY);
                  });
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

inline void drawNewMaterial(gbg::Scene& sc) {
    if (ImGui::Button("New Material")) {
        ImGui::OpenPopup("New Material");
    }

    static gbg::ShaderHandle selected = gbg::ShaderHandle();

    if (ImGui::BeginPopupModal("New Material", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        const char * def = selected ? sc.sh_mg.get(selected).getName().c_str() : "pick - shader";
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
                auto& mt = sc.mat_mg.get(mh);
                mt.setShader(selected, sh);
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.f, 1.f), "You must select a shader"); 
            }
        }

        ImGui::EndPopup();
    }
};
