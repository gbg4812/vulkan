#include <iostream>
#include <memory>
#include <print>

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

int main(int argc, char* argv[]) {
    std::span arguments(argv, argc);

    if (arguments.size() < 2) {
        std::println("Usage: app obj-file-name");
        exit(1);
    }

    const auto& scene = std::make_shared<gbg::Scene>();

    gbg::SceneRenderer renderer;

    auto sc = std::make_shared<gbg::Scene>();
    auto st = std::make_shared<gbg::SceneTree>();

    auto& mt_mg = sc->getMaterialManager();
    auto& sh_mg = sc->getShaderManager();

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::Material& mt = mt_mg.get(mth);

    gbg::ShaderHandle shh = sh_mg.create("DiffuseShader");
    gbg::Shader& sh = sh_mg.get(shh);
    sh.addParameter(gbg::ParameterTypes::VEC3_PARM);     // color
    sh.addAttribute(0, gbg::AttributeTypes::VEC3_ATTR);  // pos
    sh.addAttribute(1, gbg::AttributeTypes::VEC3_ATTR);  // normal
    sh.addAttribute(2, gbg::AttributeTypes::VEC2_ATTR);  // texture
    //
    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    mt.setShader(shh, sh);

    gbg::objLoader(arguments[1], sc.get(), st.get(), mth);

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
