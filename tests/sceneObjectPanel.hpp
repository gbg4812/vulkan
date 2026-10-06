#pragma once
#include "Light.hpp"
#include "Scene.hpp"
#include "SceneTree.hpp"
#include "imgui.h"
#include "traits/traits.hpp"

inline void drawSceneObjectPanel(gbg::Scene& sc, gbg::SceneTreeNode& sn) {
    ImGui::PushID(sn.getName().c_str());
    if (ImGui::CollapsingHeader(sn.getName().c_str())) {
        ImGui::InputFloat3("Translation", (float*)&sn.translation);
        ImGui::InputFloat3("Rotation", (float*)&sn.rotation);
        ImGui::InputFloat3("Scale", (float*)&sn.scale);

        std::visit(
            gbg::overloads{
                [&](gbg::ModelHandle handle) {
                    gbg::Model& model = sc.md_mg.get(handle);
                    if (ImGui::BeginCombo("Material",
                                          sc.mat_mg.get(model.getMaterial())
                                              .getName()
                                              .c_str())) {
                        for (auto& mt : sc.mat_mg) {
                            bool selected = model.getMaterial() == mt.getHandle();
                            if (ImGui::Selectable(
                                    mt.getName().c_str(),
                                    selected)) {
                                model.setMaterial(mt.getHandle());
                            }
                        }
                        ImGui::EndCombo();
                    }
                },
                [&](gbg::LightHandle handle) {
                    
                    gbg::Light& light = sc.lh_mg.get(handle);
                    ImGui::ColorPicker3("Light Color", (float*)&light.color);
                    ImGui::SliderFloat("Intensity",(float*)&light.intensity, 0, 100);
                    ImGui::SliderFloat("FOV",(float*)&light.fov, 0, 180);
                    
                    auto name = gbg::light_tToStr.at(light.type);
                    if(ImGui::BeginCombo("Type", name.data())) {
                        for(auto [key, value] : gbg::light_tToStr) {
                            if(ImGui::Selectable(value.data())) {
                                light.type = key;
                            }
                        }
                        ImGui::EndCombo();
                    }
                },
                [&](auto&& def) {

                },
            },
            sn.getResourceH());
    }
    ImGui::PopID();
}
