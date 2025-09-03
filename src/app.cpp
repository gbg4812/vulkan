#include <memory>

#include "Model.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "resourceLoader.hpp"

const std::string MODEL_PATH = "models/pony-cartoon/Pony_cartoon.obj";
const std::string TEXTURE_PATH =
    "models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

int main() {
    const auto& scene = std::make_shared<gbg::Scene>();
    gbg::SceneRenderer renderer;

    auto tex = gbg::loadTexture(TEXTURE_PATH);

    auto mesh =
        gbg::loadMesh("../../data/models/pony-cartoon/Pony_cartoon.obj");
    scene->meshes.push_back(mesh);

    renderer.setScene(scene);
    renderer.init();

    try {
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
