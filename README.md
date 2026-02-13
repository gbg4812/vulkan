# Vulkan Renderer

## About the project

This project is _me learning vulkan_. It started as me following the Vulkan
Tutorial and has evolved into a personal project with the objective of having _a
personal graphics toolkit to play and learn_. For now is just a _work in
progress_ of a library so my main test is the file app.cpp in the tests/
directory.

## Technologies

The toolkit uses Vulkan API as graphics API and glfw for the context and input.
It uses my other library
[SceneEquipament](https://github.com/gbg4812/SceneEquipament.git) scene input to
render.

## To build

### Prerequisits

- The vulkan sdk.

```bash
cmake -B build
cmake --build build

```

## Code Example

```cpp
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
const std::string TEXTURE_PATH =
    "./data/models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

int main(int argc, char* argv[]) {
    std::span arguments(argv, argc);

    if (arguments.size() < 2) {
        std::println("Usage: app obj-file-name");
        exit(1);
    }

    // Scene creation
    const auto& scene = std::make_shared<gbg::Scene>();

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
    //
    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    mt.setShader(shh, sh);

    gbg::objLoader(arguments[1], sc.get(), st.get(), mth);

    // Creates the renderer and assigns the scene
    gbg::SceneRenderer renderer;
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
```

## Organitzation

Directories:

- src/ where all the source code is.
- src/external repositoris of external dependencies like stb_image.
- src/vk_utils vulkan utility functions and structures (my be some day I will
  separate them into their own library).
