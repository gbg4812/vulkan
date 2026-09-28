#pragma once
#include <iostream>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "FileWatcher.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneRenderer.hpp"
#include "io_utils/watcher.hpp"
#include "loaders/texLoader.hpp"
#include "resourcesUpdate.hpp"
#include "shaderReflexion.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context)
        : renderer(context), file_w(file_m, dep_tree) {
        // default texture
        auto& def_tex = scene.tx_mg.create("DefaultTexture");
        loadTexture("data/models/RendererResources/DefaultTexture.png",
                    def_tex);
        scene.defaults.texture = def_tex.getHandle();
        gbg::createRepresentative(dep_tree, def_tex,
                                  gbg::ResourceTypes::TEXTURE,
                                  gbg::SObjFlags::NEW);

        auto& sh_mg = scene.getShaderManager();

        // Shader Creation
        auto& sh = sh_mg.create("DefaultShader");
        scene.defaults.shader = sh.getHandle();
        gbg::createRepresentative(dep_tree, sh, gbg::ResourceTypes::SHADER,
                                  gbg::SObjFlags::NEW);

        // CONTINUE
        file_w.createFile("./data/shaders/default.frag");
        WatchedFile frag_f("./data/shaders/default.frag", &file_m, &dep_tree);
        WatchedFile frag_v("./data/shaders/default.vert", &file_m, &dep_tree);

        auto res = gbg::setGlslShaderCode(
            sh, {"./data/shaders/default.frag", "./data/shaders/default.vert"});
        if (not res.first) {
            std::cout << res.second << std::endl;
            exit(EXIT_FAILURE);
        }

        // TODO(guillem): set dependent take handles to node trees and
        gbg::setDependent(dep_tree, sh, gbg::SObjFlags::CODE_M, frag_f.h, gbg);

        watch({file_m.get(frag_f.h).path}, WatchEvents::MODFY, frag_f);

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
        gbg::setDependent(dep_tree, mt, gbg::SObjFlags::PARAMETER_INTERFACE_M,
                          sh, gbg::SObjFlags::CODE_M);
        gbg::setDependent(
            dep_tree, mt, gbg::SObjFlags::TEXTURE_PARAMETER_VALUE_M,
            scene.tx_mg.get(scene.defaults.texture), gbg::SObjFlags::NEW);

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
    gbg::FileManager file_m;
    FileWatcher file_w;
};
