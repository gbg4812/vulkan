#pragma once
#include "Scene.hpp"
#include "SceneTree.hpp"
#include "imgui.h"
#include "traits/traits.hpp"

inline void drawSceneObjectPanel(gbg::Scene& sc, gbg::SceneTreeNode& sn) {
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
                        for (auto mth : sc.mat_mg) {
                            bool selected = model.getMaterial() == mth;
                            if (ImGui::Selectable(
                                    sc.mat_mg.get(mth).getName().c_str(),
                                    selected)) {
                                model.setMaterial(mth);
                            }
                        }
                        ImGui::EndCombo();
                    }
                },
                [&](gbg::LightHandle handle) {
                    gbg::Light& light = sc.lh_mg.get(handle);
                    ImGui::ColorPicker3("Light Color", (float*)&light.color);
                },
                [&](auto&& def) {

                },
            },
            sn.getResourceH());
    }
}
