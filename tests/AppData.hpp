#pragma once
#include "RendererContext.hpp"
#include "SceneRenderer.hpp"
#include "Texture.hpp"
#include "loaders/texLoader.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context) : renderer(context) {
        auto tx_def = scene.tx_mg.create("DefaultTexture");
        loadTexture("data/models/RendererResources/DefaultTexture.png", &scene,  tx_def);
    }
    bool ui_mode;
    gbg::SceneRenderer renderer;
    gbg::Scene scene;
};
