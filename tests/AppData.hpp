#pragma once
#include <cstdlib>
#include <expected>
#include <iostream>
#include <stdexcept>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "FileWatcher.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
#include "Shader.hpp"
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

        auto vert_f = file_w.createFile("./data/shaders/default.vert");
        auto frag_f = file_w.createFile("./data/shaders/default.frag");

        if (frag_f and vert_f) {
            auto res_v = gbg::setShaderCode(sh, vert_f.value()->path,
                                            gbg::ShaderTypes::VERTEX);
            auto res_f = gbg::setShaderCode(sh, frag_f.value()->path,
                                            gbg::ShaderTypes::FRAGMENT);
            if (res_v or res_f) {
                if (res_v) std::cout << res_v.value() << std::endl;
                if (res_f) std::cout << res_f.value() << std::endl;
                throw std::runtime_error("Filed to load default shader!");
            }
            gbg::setDependent(
                dep_tree, sh.representative, gbg::SObjFlags::CODE_M,
                frag_f.value()->representative, FileFlags::FILE_M);
            gbg::setDependent(
                dep_tree, sh.representative, gbg::SObjFlags::CODE_M,
                vert_f.value()->representative, FileFlags::FILE_M);
        }

        // Material Creation
        auto& mt_mg = scene.getMaterialManager();

        gbg::Material& mt = mt_mg.create("DefaultMaterial");
        scene.defaults.material = mt.getHandle();

        gbg::createRepresentative(dep_tree, mt, gbg::ResourceTypes::MATERIAL,
                                  gbg::SObjFlags::NEW);

        mt.setShader(scene.defaults.shader);

        gbg::setDependent(dep_tree, mt.representative, gbg::SObjFlags::DELETED,
                          sh.representative, gbg::SObjFlags::DELETED);
        gbg::setDependent(dep_tree, mt.representative,
                          gbg::SObjFlags::PARAMETER_INTERFACE_M,
                          sh.representative, gbg::SObjFlags::CODE_M);
        gbg::setDependent(
            dep_tree, mt.representative,
            gbg::SObjFlags::TEXTURE_PARAMETER_VALUE_M,
            scene.tx_mg.get(scene.defaults.texture).representative,
            gbg::SObjFlags::NEW);

        // Camera
        auto& st_mg = scene.getSceneTreeManager();
        auto& cm_mg = scene.getCameraManager();
        scene.defaults.camera = cm_mg.create("Camera").getHandle();
        gbg::SceneTreeNode& cm_n = st_mg.create("DefaultCamera");
        cm_n.translation += glm::vec3{12.0f, 5.0f, -3.0f};
        cm_n.rotation += glm::vec3{-0.3f, 1.92f, 0.0f};
        cm_n.setResource(scene.defaults.camera);
        st_mg.prependChild(scene.root, cm_n.getHandle());
        scene.active_camera = cm_n.getHandle();

        // Light
        scene.defaults.light = scene.lh_mg.create("Light").getHandle();
        gbg::SceneTreeNode& lh_n = st_mg.create("DefaultLigth");
        lh_n.setResource(scene.defaults.light);
        lh_n.translation = {5, 2, -5};
        lh_n.rotation.y = 130;
        st_mg.prependChild(scene.root, lh_n.getHandle());

        gbg::createRepresentative(dep_tree, st_mg.get(scene.root),
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
