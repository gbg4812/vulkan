#include <iostream>
#include <memory>

#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "SceneTree.hpp"
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

    gbg::objLoader(MODEL_PATH, *sc.get(), st.get());

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
