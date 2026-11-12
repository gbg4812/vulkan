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
#include "extras/File.hpp"
#include "loaders/texLoader.hpp"
#include "resourcesUpdate.hpp"
#include "shaderUtils.hpp"

#define DATA_PATH  "./data/"

struct AppData {
    AppData(const gbg::RendererContext& context)
        : renderer(context),
          scene(std::make_shared<gbg::Scene>()),
          dep_tree(std::make_shared<gbg::DependencyTreeManager>()),
          file_w(file_m, *dep_tree) {
        // default texture
        auto& def_tex = scene->tx_mg.create("DefaultTexture");
        auto res = loadTexture("./data/textures/DefaultTexture.png", def_tex); 
        if(not res) {
            std::cout << res.error();
            throw std::runtime_error("Failed to load default texture!");
        }
        scene->defaults.texture = def_tex.getHandle();
        gbg::createRepresentative(*dep_tree, def_tex,
                                  gbg::ResourceTypes::TEXTURE,
                                  gbg::SObjFlags::NEW);

        auto& sh_mg = scene->getShaderManager();

        // Shader Creation
        {
            auto& sh = sh_mg.create("DefaultShader");
            scene->defaults.shader = sh.getHandle();
            gbg::createRepresentative(*dep_tree, sh, gbg::ResourceTypes::SHADER,
                                      gbg::SObjFlags::NEW);

            auto res = file_w.createFile("./data/shaders/default.slang")
                           .and_then([&](gbg::FileHandle fh) {
                               sh.appendCodeFile(fh);
                               return gbg::compileShader(file_m, sh);
                           })
                           .transform([&](std::string diagnostics) {
                               std::cout << diagnostics << std::endl;
                               gbg::setDependent(
                                   *dep_tree, sh.representative,
                                   gbg::SObjFlags::CODE_M,
                                   file_m.get(sh.getCodeFile(0)).representative,
                                   FileFlags::FILE_M);
                           })
                           .transform_error([](std::string error) {
                               std::cout << error << std::endl;
                               return error;
                           });

            if (not res) {
                throw std::runtime_error("Failed to load default shader");
            }
        }

        {
            auto& sh = sh_mg.create("ObjShader");
            gbg::createRepresentative(*dep_tree, sh, gbg::ResourceTypes::SHADER,
                                      gbg::SObjFlags::NEW);
            auto res =
                file_w.createFile("./data/shaders/obj_complient.slang")
                    .and_then([&](gbg::FileHandle fh) {
                        sh.appendCodeFile(fh);
                        return gbg::compileShader(file_m, sh);
                    })
                    .transform([&](std::string diagnostics) {
                        std::cout << diagnostics << std::endl;
                        gbg::setDependent(
                            *dep_tree, sh.representative,
                            gbg::SObjFlags::CODE_M,
                            file_m.get(sh.getCodeFile(0)).representative,
                            FileFlags::FILE_M);
                    })
                    .transform_error([](std::string error) {
                        std::cout << error << std::endl;
                        return error;
                    });
        }

        // Material Creation
        auto& mt_mg = scene->getMaterialManager();

        gbg::Material& mt = mt_mg.create("DefaultMaterial");
        scene->defaults.material = mt.getHandle();

        gbg::createRepresentative(*dep_tree, mt, gbg::ResourceTypes::MATERIAL,
                                  gbg::SObjFlags::NEW);

        mt.setShader(scene->defaults.shader);

        gbg::setDependent(
            *dep_tree, mt.representative, gbg::SObjFlags::PARAMETER_INTERFACE_M,
            scene->getDefaultShader().representative, gbg::SObjFlags::CODE_M);
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

    }
    bool ui_mode = false;
    glm::vec<2, double> cursor_pos = {};
    gbg::SceneRenderer renderer;
    std::shared_ptr<gbg::Scene> scene;
    std::shared_ptr<gbg::DependencyTreeManager> dep_tree;
    gbg::FileManager file_m;
    FileWatcher file_w;
};
