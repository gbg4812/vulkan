#pragma once
#include <string>

#include "AppData.hpp"
#include "DependencyTreeFunctions.hpp"
#include "Model.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
#include "imgui.h"
#include "loaders/objLoader.hpp"
#include "nfd.h"
#include "resourcesUpdate.hpp"

inline void drawCreateObject(AppData& app) {
    gbg::Scene& scene = *app.scene;

    static std::filesystem::path chosen_path;

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
                ImGui::OpenPopup("Chose Base Shader");
            }
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginPopupModal("Chose Base Shader")) {
        for (auto& sh : app.scene->sh_mg) {
            if (ImGui::Selectable(sh.getName().c_str())) {
                auto sthl =
                    gbg::objLoader(chosen_path, &scene, scene.root,
                                   sh.getHandle(), scene.defaults.material);
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
