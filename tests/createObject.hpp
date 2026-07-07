#pragma once
#include <string>

#include "Scene.hpp"
#include "imgui.h"
#include "loaders/objLoader.hpp"
#include "nfd.h"

inline void drawCreateObject(gbg::Scene& scene) {

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
                gbg::objLoader(outpath, &scene, scene.root, scene.defaults.material);
                NFD_FreePathU8(outpath);
            }
        }
        
        ImGui::EndMenu();
    }
    
    
    
}
