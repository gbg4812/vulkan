#include <iostream>
#include <memory>

#include "Mesh.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
#include "Shader.hpp"
#include "loaders/objLoader.hpp"

const std::string MODEL_PATH = "./data/models/EasyModels/scene.obj";
// const std::string MODEL_PATH = "./data/models/pony-cartoon/Pony_cartoon.obj";
const std::string TEXTURE_PATH =
    "./data/models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

int main() {
    const auto& scene = std::make_shared<gbg::Scene>();
    gbg::SceneRenderer renderer;
    auto sc = std::make_shared<gbg::Scene>();
    auto st = std::make_shared<gbg::SceneTree>();

    auto& mt_mg = sc->getMaterialManager();
    auto& sh_mg = sc->getShaderManager();

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::Material& mt = mt_mg.get(mth);

    gbg::ShaderHandle shh = sh_mg.create("DiffuseShader");
    gbg::Shader sh = sh_mg.get(shh);
    sh.addParameter(gbg::ParameterTypes::VEC3_PARM);     // color
    sh.addAttribute(0, gbg::AttributeTypes::VEC3_ATTR);  // pos

    mt.setShader(shh, sh);

    gbg::objLoader(MODEL_PATH, sc.get(), st.get(), mth);

    renderer.setScene(sc, st);
    renderer.init();

    try {
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
