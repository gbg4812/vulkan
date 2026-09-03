#pragma once
#include <iostream>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneRenderer.hpp"
#include "io_utils/watcher.hpp"
#include "loaders/texLoader.hpp"
#include "resourcesUpdate.hpp"
#include "shaderReflexion.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context) : renderer(context) {
        // default texture
        scene.defaults.texture = scene.tx_mg.create("DefaultTexture");
        loadTexture("data/models/RendererResources/DefaultTexture.png", &scene,
                    scene.defaults.texture);
        gbg::createRepresentative(dep_tree, scene.defaults.texture, scene.tx_mg,
                                  gbg::ResourceTypes::TEXTURE,
                                  gbg::SObjFlags::NEW);

        auto& sh_mg = scene.getShaderManager();

        // Shader Creation
        scene.defaults.shader = sh_mg.create("DefaultShader");
        gbg::Shader& sh = sh_mg.get(scene.defaults.shader);
        gbg::createRepresentative(dep_tree, scene.defaults.shader, sh_mg,
                                  gbg::ResourceTypes::SHADER,
                                  gbg::SObjFlags::NEW);

        auto res =
            gbg::setShaderCode(sh, "./data/shaders/shader.vert", gbg::VERTEX);
        if (not res.first) {
            std::cout << res.second << std::endl;
            exit(EXIT_FAILURE);
        }
        res =
            gbg::setShaderCode(sh, "./data/shaders/shader.frag", gbg::FRAGMENT);
        if (not res.first) {
            std::cout << res.second << std::endl;
            exit(EXIT_FAILURE);
        }

        dep_tree.propagateChange(sh.representative,
                                 gbg::SObjFlags::DIRTY_SHADER_CODE);

        watch({"./data/shaders/shader.frag", "./data/shaders/shader.vert"},
              (uint32_t)WatchEvents::MODFY, [&]() {
                  auto res = gbg::setShaderCode(
                      sh, "./data/shaders/shader.vert", gbg::VERTEX);
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

                  dep_tree.propagateChange(sh.representative,
                                           gbg::SObjFlags::DIRTY_SHADER_CODE);
              });

        // Material Creation
        auto& mt_mg = scene.getMaterialManager();

        scene.defaults.material = mt_mg.create("DefaultMaterial");
        gbg::Material& mt = mt_mg.get(scene.defaults.material);

        gbg::createRepresentative(dep_tree, scene.defaults.material, mt_mg,
                                  gbg::ResourceTypes::MATERIAL,
                                  gbg::SObjFlags::NEW);

        mt.setShader(scene.defaults.shader);

        gbg::setDependent(dep_tree, mt, gbg::SObjFlags::DELETED, sh,
                          gbg::SObjFlags::DELETED);
        gbg::setDependent(dep_tree, mt, gbg::SObjFlags::SHADER_CHANGED, sh,
                          gbg::SObjFlags::DIRTY_SHADER_CODE);
        gbg::setDependent(dep_tree, mt, gbg::SObjFlags::TEXTURE_CHANGED,
                          scene.tx_mg.get(scene.defaults.texture),
                          gbg::SObjFlags::NEW);

        // Camera
        auto& st_mg = scene.getSceneTreeManager();
        auto& cm_mg = scene.getCameraManager();
        scene.defaults.camera = cm_mg.create("Camera");
        gbg::SceneTreeHandle cm_nh = st_mg.create("DefaultCamera");
        st_mg.get(cm_nh).translation += glm::vec3{12.0f, 5.0f, -3.0f};
        st_mg.get(cm_nh).rotation += glm::vec3{-0.3f, 1.92f, 0.0f};
        st_mg.get(cm_nh).setResource(scene.defaults.camera);
        st_mg.prependChild(scene.root, cm_nh);
        scene.active_camera = cm_nh;

        // Light
        scene.defaults.light = scene.lh_mg.create("Light");
        gbg::SceneTreeHandle lh_nh = st_mg.create("DefaultLigth");
        st_mg.get(lh_nh).setResource(scene.defaults.light);
        st_mg.get(lh_nh).translation = {5, 2, -5};
        st_mg.get(lh_nh).rotation.y = 130;
        st_mg.prependChild(scene.root, lh_nh);

        gbg::createRepresentative(dep_tree, scene.root, scene.st_mg,
                                  gbg::ResourceTypes::SCENE_TREE_NODE,
                                  gbg::SObjFlags::NEW);
    }
    bool ui_mode = false;
    glm::vec<2, double> cursor_pos = {};
    gbg::SceneRenderer renderer;
    gbg::Scene scene;
    gbg::DependencyTreeManager dep_tree;
};
