#pragma once

#include <string>

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

inline void loadObject(AppData& app, const std::filesystem::path& path, const gbg::Shader& sh) {
    auto mat_path = path;
    mat_path.replace_extension("mtl");
    auto res =
        gbg::objLoadMaterials(mat_path, *app.scene, sh.getHandle());
    for (auto [_, tx_h] : res->textures) {
        auto& tx = app.scene->tx_mg.get(tx_h);
        gbg::createRepresentative(*app.dep_tree, tx,
                                  gbg::ResourceTypes::TEXTURE,
                                  gbg::SObjFlags::NEW);
    }
    for (auto [_, mt_h] : res->materials) {
        auto& mt = app.scene->mat_mg.get(mt_h);
        gbg::createRepresentative(*app.dep_tree, mt,
                                  gbg::ResourceTypes::MATERIAL,
                                  gbg::SObjFlags::NEW);
        gbg::setDependent(*app.dep_tree, mt.representative,
                          gbg::SObjFlags::PARAMETER_INTERFACE_M, sh.representative,
                          gbg::SObjFlags::CODE_M);
    }
    auto& object = app.scene->st_mg.create(mat_path.filename().replace_extension());
    app.scene->st_mg.prependChild(app.scene->root, object.getHandle());
    
    auto sthl =
        gbg::objLoadMeshes(path, app.scene.get(), object.getHandle(), res->materials);
    for (auto sth : sthl) {
        auto& n = app.scene->st_mg.get(sth);
        gbg::ModelHandle h =
            std::get<gbg::ModelHandle>(n.getResourceH());
        auto msh = app.scene->md_mg.get(h).getMesh();
        gbg::createRepresentative(
            *app.dep_tree, app.scene->ms_mg.get(msh),
            gbg::ResourceTypes::MESH, gbg::SObjFlags::NEW);
    }
}

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
                loadObject(app, chosen_path, sh);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}
