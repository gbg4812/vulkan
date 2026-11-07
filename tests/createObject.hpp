#pragma once
#include <fcntl.h>

#include <string>
#include <variant>

#include "AppData.hpp"
#include "DependencyTreeFunctions.hpp"
#include "Model.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
#include "Texture.hpp"
#include "imgui.h"
#include "loaders/objLoader.hpp"
#include "nfd.h"
#include "resourcesUpdate.hpp"

inline void drawCreateObject(AppData& app) {
    gbg::Scene& scene = *app.scene;

    static std::filesystem::path chosen_path;
    bool open_popup = false;

    if (ImGui::BeginMenu("Add")) {
        if (ImGui::MenuItem("Light")) {
            auto& l = scene.lh_mg.create("Light");
            auto& st = scene.st_mg.create("Light" + std::to_string(l.getRID()));
            scene.st_mg.prependChild(scene.root, st.getHandle());
            st.setResource(l.getHandle());
        }

        if (ImGui::MenuItem("Load Model")) {
            nfdu8char_t* outpath = nullptr;
            nfdopendialognargs_t args = {0};
            nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
            if (res == NFD_OKAY) {
                chosen_path = outpath;
                NFD_FreePathU8(outpath);
                open_popup = true;
            }
        }

        ImGui::EndMenu();
    }

    if (open_popup) {
        ImGui::OpenPopup("Chose Base Shader");
        open_popup = false;
    }

    if (ImGui::BeginPopupModal("Chose Base Shader", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        for (auto& sh : app.scene->sh_mg) {
            if (ImGui::Selectable(sh.getName().c_str())) {
                auto mat_path = chosen_path;
                mat_path.replace_extension("mtl");
                auto [mats, texs] =
                    gbg::objLoadMaterials(mat_path, scene, sh.getHandle());
                for (auto [_, tx_h] : texs) {
                    auto& tx = app.scene->tx_mg.get(tx_h);
                    gbg::createRepresentative(*app.dep_tree, tx,
                                              gbg::ResourceTypes::TEXTURE,
                                              gbg::SObjFlags::NEW);
                }
                for (auto [_, mt_h] : mats) {
                    auto& mt = app.scene->mat_mg.get(mt_h);
                    gbg::createRepresentative(*app.dep_tree, mt,
                                              gbg::ResourceTypes::MATERIAL,
                                              gbg::SObjFlags::NEW);
                    gbg::setDependent(*app.dep_tree, mt.representative,
                                      gbg::SObjFlags::CODE_M, sh.representative,
                                      gbg::SObjFlags::PARAMETER_INTERFACE_M);
                }
                auto sthl =
                    gbg::objLoadMeshes(chosen_path, &scene, scene.root, mats);
                for (auto sth : sthl) {
                    auto& n = scene.st_mg.get(sth);
                    gbg::ModelHandle h =
                        std::get<gbg::ModelHandle>(n.getResourceH());
                    auto msh = scene.md_mg.get(h).getMesh();
                    gbg::createRepresentative(
                        *app.dep_tree, scene.ms_mg.get(msh),
                        gbg::ResourceTypes::MESH, gbg::SObjFlags::NEW);
                }
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}
