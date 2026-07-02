#pragma once
#include "srMaterial.hpp"
#include "srMesh.hh"
#include "srShader.hpp"
#include "srTexture.hpp"
#include "srLight.hpp"
#include "Scene.hpp"

namespace gbg {
    struct InternalSceneData {
        srMaterialManager srmat_mg;
        srShaderManager srsh_mg;
        srTextureManager srtx_mg;
        srMeshManager srmsh_mg;
        srLightManager srlight_mg;

        Scene* scene;
    };
}
