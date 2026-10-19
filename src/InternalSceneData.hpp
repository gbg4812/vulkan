#pragma once
#include "DependencyTree.hpp"
#include "Scene.hpp"
#include "srMaterial.hpp"
#include "srMesh.hh"
#include "srShader.hpp"
#include "srTexture.hpp"

namespace gbg {
struct InternalSceneData {
    srMaterialManager srmat_mg;
    srShaderManager srsh_mg;
    srTextureManager srtx_mg;
    srMeshManager srmsh_mg;

    std::shared_ptr<Scene> scene = nullptr;
    std::shared_ptr<DependencyTreeManager> dep_tree = nullptr;
    std::shared_ptr<FileManager> file_m = nullptr;
};
}  // namespace gbg
