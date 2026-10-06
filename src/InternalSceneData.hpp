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

    Scene* scene;

    DependencyTreeManager* dep_tree;
};
}  // namespace gbg
