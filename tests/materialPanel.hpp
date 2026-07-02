#pragma once
#include <ranges>
#include "Scene.hpp"
#include "imgui.h"
#include "loaders/texLoader.hpp"
#include "nfd.h"

void drawMaterialPanel(gbg::Scene& sc, gbg::Material& mat) {
    if (ImGui::CollapsingHeader(mat.getName().c_str())) {
        for (auto [num, value] : mat.getValues() | std::views::enumerate) {
            if (const gbg::TextureHandle* h =
                    std::get_if<gbg::TextureHandle>(&value)) {
                auto& tex = sc.tx_mg.get(*h);
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
            nfdu8char_t* outpath = nullptr;
            static char buff[1024] = "";
            static char name[64] = "";
            if (ImGui::Button("Search")) {
                nfdopendialognargs_t args = {0};
                nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
                if (res == NFD_OKAY) {
                    if (strlen(outpath) < sizeof(buff)) strcpy(buff, outpath);
                    NFD_FreePathU8(outpath);
                }
            }

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
}
