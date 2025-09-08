#pragma once
#include <memory>
#include <set>

#include "ElementUtils.hpp"
#include "Model.hpp"
#include "Shader.hpp"

namespace gbg {

class Scene {
   public:
    Scene() {};

    int createMesh() {
        auto nmesh = std::make_shared<Mesh>();
        int idx = firstFreeIndex(_meshes);
        return idx;
    }

    int createModel(int mat, int mesh) {
        auto nmodel = std::make_shared<Model>(mat, mesh);
        int idx = firstFreeIndex(_models);
        return idx;
    }

    int createShader(path_t vert_shader, path_t frag_shader) {
        auto nshader = std::make_shared<Shader>(vert_shader, frag_shader);
        int idx = firstFreeIndex(_shaders);
        return idx;
    }

    int createMaterial() {
        auto nmaterial = std::make_shared<Material>();
        int idx = firstFreeIndex(_materials);
        return idx;
    }

    const std::set<std::shared_ptr<Model>>& getModels() const {
        return _models;
    }

    const std::set<std::shared_ptr<Shader>>& getShaders() const {
        return _shaders;
    }

    const std::set<std::shared_ptr<Mesh>>& getMeshes() const { return _meshes; }

    const std::set<std::shared_ptr<Material>>& getMaterials() const {
        return _materials;
    }

   private:
    std::set<std::shared_ptr<Model>> _models;
    std::set<std::shared_ptr<Shader>> _shaders;
    std::set<std::shared_ptr<Mesh>> _meshes;
    std::set<std::shared_ptr<Material>> _materials;
};

}  // namespace gbg
