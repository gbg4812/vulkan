#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "DependencyTree.hpp"
#include "DependencyTreeFunctions.hpp"
#include "FileWatcher.hpp"
#include "Light.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
#include "Shader.hpp"
#include "loaders/objLoader.hpp"
#include "loaders/texLoader.hpp"
#include "resourcesUpdate.hpp"
#include "shaderUtils.hpp"

struct AppData {
    AppData(const gbg::RendererContext& context, std::span<char*> argv)
        : renderer(context),
          scene(std::make_shared<gbg::Scene>()),
          dep_tree(std::make_shared<gbg::DependencyTreeManager>()),
          file_w(file_m, *dep_tree) {
        // default texture
        auto& def_tex = scene->tx_mg.create("DefaultTexture");
        if (not loadTexture("./data/textures/DefaultTexture.png", def_tex)) {
            throw std::runtime_error("Failed to load default texture!");
        }
        scene->defaults.texture = def_tex.getHandle();
        gbg::createRepresentative(*dep_tree, def_tex,
                                  gbg::ResourceTypes::TEXTURE,
                                  gbg::SObjFlags::NEW);

        auto& sh_mg = scene->getShaderManager();

        // Shader Creation
        auto& sh = sh_mg.create("DefaultShader");
        scene->defaults.shader = sh.getHandle();
        gbg::createRepresentative(*dep_tree, sh, gbg::ResourceTypes::SHADER,
                                  gbg::SObjFlags::NEW);

        auto watch_res =
            file_w.createFile("./data/shaders/slang/default.slang");
        if (watch_res) {
            sh.addCodeFile(watch_res.value(), gbg::ShaderTypes::COMBINED);
            auto compile_res = gbg::compileShader(file_m, sh);
            if (not compile_res) {
                throw std::runtime_error(compile_res.error());
            }
            std::cout << compile_res.value() << std::endl;

            gbg::setDependent(*dep_tree, sh.representative,
                              gbg::SObjFlags::CODE_M,
                              file_m.get(watch_res.value()).representative,
                              FileFlags::FILE_M);
        }

        // Material Creation
        auto& mt_mg = scene->getMaterialManager();

        gbg::Material& mt = mt_mg.create("DefaultMaterial");
        scene->defaults.material = mt.getHandle();

        gbg::createRepresentative(*dep_tree, mt, gbg::ResourceTypes::MATERIAL,
                                  gbg::SObjFlags::NEW);

        mt.setShader(scene->defaults.shader);

        gbg::setDependent(*dep_tree, mt.representative,
                          gbg::SObjFlags::PARAMETER_INTERFACE_M,
                          sh.representative, gbg::SObjFlags::CODE_M);
        gbg::setDependent(
            *dep_tree, mt.representative,
            gbg::SObjFlags::TEXTURE_PARAMETER_VALUE_M,
            scene->tx_mg.get(scene->defaults.texture).representative,
            gbg::SObjFlags::NEW);

        // Camera
        auto& st_mg = scene->getSceneTreeManager();
        auto& cm_mg = scene->getCameraManager();
        auto& cm = cm_mg.create("Camera");
        gbg::createRepresentative(*dep_tree, cm, gbg::ResourceTypes::CAMERA,
                                  gbg::SObjFlags::NEW);
        cm.fov = 60;
        cm.znear = 1;
        cm.zfar = 200;
        scene->defaults.camera = cm.getHandle();
        gbg::SceneTreeNode& cm_n = st_mg.create("DefaultCamera");
        cm_n.translation = glm::vec3{15.0f, 5.0f, -5.0f};
        cm_n.rotation = glm::vec3{-22.f, 107.f, 0.0f};
        cm_n.setResource(scene->defaults.camera);
        st_mg.prependChild(scene->root, cm_n.getHandle());
        scene->active_camera = cm_n.getHandle();

        // Light
        auto& lh = scene->lh_mg.create("Light");
        scene->defaults.light = lh.getHandle();
        lh.type = gbg::LightType::DIRECTIONAL;
        lh.intensity = 1.0f;
        gbg::SceneTreeNode& lh_n = st_mg.create("DefaultLigth");
        lh_n.setResource(scene->defaults.light);
        lh_n.translation = {0, 0, 0};
        lh_n.rotation = glm::vec3{-60.f, 180.f, 0.f};
        st_mg.prependChild(scene->root, lh_n.getHandle());

        gbg::createRepresentative(*dep_tree, st_mg.get(scene->root),
                                  gbg::ResourceTypes::SCENE_TREE_NODE,
                                  gbg::SObjFlags::NEW);

        if (argv.size() > 1) {
            auto res = gbg::objLoader(argv[1], scene.get(), scene->root,
                                      scene->defaults.material);
            for (auto sth : res) {
                auto& n = scene->st_mg.get(sth);
                gbg::ModelHandle h =
                    std::get<gbg::ModelHandle>(n.getResourceH());
                auto msh = scene->md_mg.get(h).getMesh();
                gbg::createRepresentative(*dep_tree, scene->ms_mg.get(msh),
                                          gbg::ResourceTypes::MESH,
                                          gbg::SObjFlags::NEW);
            }
        }
    }
    bool ui_mode = false;
    glm::vec<2, double> cursor_pos = {};
    gbg::SceneRenderer renderer;
    std::shared_ptr<gbg::Scene> scene;
    std::shared_ptr<gbg::DependencyTreeManager> dep_tree;
    gbg::FileManager file_m;
    FileWatcher file_w;
};
