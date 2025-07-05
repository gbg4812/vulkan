#include <memory>
#include <vector>

#include "Model.hpp"
#include "Shader.hpp"

#pragma once

namespace gbg {

class Scene {
   public:
    std::vector<std::shared_ptr<MeshAttribute>> attributes;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Model>> models;
    std::vector<std::shared_ptr<Shader>> shaders;
    std::vector<std::shared_ptr<Material>> materials;
};

}  // namespace gbg
