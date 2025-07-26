#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "resourceLoader.hpp"

const std::string MODEL_PATH = "models/pony-cartoon/Pony_cartoon.obj";
const std::string TEXTURE_PATH =
    "models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

int main() {
    gbg::Scene scene;
    gbg::SceneRenderer app;
    app.init();

    auto tex = gbg::loadTexture(TEXTURE_PATH);

    auto mesh =
        gbg::loadMesh("../../data/models/pony-cartoon/Pony_cartoon.obj");

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
