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
    gbg::Scene& scene = app.scene;

    if(ImGui::BeginMenu("Add")) {

        if(ImGui::MenuItem("Light")) {
            auto lh = scene.lh_mg.create("Light");
            auto sth = scene.st_mg.create("Light" + std::to_string(scene.lh_mg.nextID()));
            scene.st_mg.prependChild(scene.root, sth);
            scene.st_mg.get(sth).setResource(lh);
        }

        if (ImGui::MenuItem("Load Model")) {
            nfdu8char_t* outpath = nullptr;
            nfdopendialognargs_t args = {0};
            nfdresult_t res = NFD_OpenDialogU8_With(&outpath, &args);
            if (res == NFD_OKAY) {
                auto sthl = gbg::objLoader(outpath, &scene, scene.root, scene.defaults.material);
                for(auto sth : sthl) {
                    auto& n = scene.st_mg.get(sth);
                    gbg::ModelHandle h = std::get<gbg::ModelHandle>(n.getResourceH());
                    auto msh = scene.md_mg.get(h).getMesh();
                    gbg::createRepresentative(app.dep_tree, msh, scene.ms_mg, gbg::ResourceTypes::MESH, gbg::SObjFlags::NEW);
                }
                NFD_FreePathU8(outpath);
            }
        }

        ImGui::EndMenu();
    }



}
