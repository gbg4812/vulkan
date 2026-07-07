#pragma once
#include "RendererContext.hpp"
#include "SceneRenderer.hpp"
#include "Texture.hpp"
#include "io_utils/watcher.hpp"
#include "loaders/texLoader.hpp"
#include "shaderReflexion.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context) : renderer(context) {
        // default texture
        scene.defaults.texture = scene.tx_mg.create("DefaultTexture");
        loadTexture("data/models/RendererResources/DefaultTexture.png", &scene,  scene.defaults.texture);
        
        auto& sh_mg = scene.getShaderManager();
    
        // Shader Creation
        scene.defaults.shader = sh_mg.create("DefaultShader");
        gbg::Shader& sh = sh_mg.get(scene.defaults.shader);
    
        auto res =
            gbg::setShaderCode(sh, "./data/shaders/shader.vert", gbg::VERTEX);
        if (not res.first) {
            std::cout << res.second << std::endl;
            exit(EXIT_FAILURE);
        }
        res = gbg::setShaderCode(sh, "./data/shaders/shader.frag", gbg::FRAGMENT);
        if (not res.first) {
            std::cout << res.second << std::endl;
            exit(EXIT_FAILURE);
        }
        gbg::reflectShader(sh);
    
        watch({"./data/shaders/shader.frag", "./data/shaders/shader.vert"},
              (uint32_t)WatchEvents::MODFY, [&]() {
                  auto res = gbg::setShaderCode(sh, "./data/shaders/shader.vert",
                                                gbg::VERTEX);
                  if (not res.first) {
                      std::cout << res.second << std::endl;
                  } else {
                      std::cout << "Shader recompiled successfuly" << std::endl;
                  }
                  res = gbg::setShaderCode(sh, "./data/shaders/shader.frag",
                                           gbg::FRAGMENT);
                  if (not res.first) {
                      std::cout << res.second << std::endl;
                  } else {
                      std::cout << "Shader recompiled successfuly" << std::endl;
                  }
    
                  gbg::reflectShader(sh);
    
                  for (gbg::MaterialHandle mh : scene.mat_mg) {
                      scene.mat_mg.get(mh).setShader(scene.defaults.shader, sh);
                      scene.mat_mg.get(mh).setFlags(gbg::ResourceFlags::DIRTY);
                  }
                  sh.setFlags(gbg::ResourceFlags::DIRTY);
              });
    
        // Material Creation
        auto& mt_mg = scene.getMaterialManager();
    
        scene.defaults.material = mt_mg.create("DefaultMaterial");
        gbg::Material& mt = mt_mg.get(scene.defaults.material);
    
        mt.setShader(scene.defaults.shader, sh);
    }
    bool ui_mode;
    gbg::SceneRenderer renderer;
    gbg::Scene scene;
};
