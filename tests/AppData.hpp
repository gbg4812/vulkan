#pragma once
#include "RendererContext.hpp"
#include "SceneRenderer.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context) : renderer(context) {
    }
    bool ui_mode;
    gbg::SceneRenderer renderer;
    gbg::Scene scene;
};
