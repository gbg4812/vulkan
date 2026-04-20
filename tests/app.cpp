#include <iostream>
#include <memory>
#include <ostream>
#include <span>

#include "GlfwCreateRendererContext.hpp"
#include "Resource.hpp"
#include "SceneTree.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "loaders/objLoader.hpp"
#include "vk_utils/vkInstance.hh"

const std::string TEXTURE_PATH =
    "./data/models/pony-cartoon/textures/Body_dDo_d_orange.jpeg";

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app =
        reinterpret_cast<gbg::SceneRenderer*>(glfwGetWindowUserPointer(window));

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    app->resizeSwapchain(width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                 int mods) {
    auto app =
        reinterpret_cast<gbg::SceneRenderer*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        std::cout << "W key pressed" << std::endl;
    }
}

void setupGlfwCallbacks(GLFWwindow* window, void* userPointer) {
    glfwSetWindowUserPointer(window, userPointer);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
}

GLFWwindow* createWindow(int width, int height, std::string name) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window =
        glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    return window;
}

int main(int argc, char* argv[]) {
    std::span arguments(argv, argc);

    if (arguments.size() < 2) {
        std::cout << "Usage: app obj-file-name" << std::endl;
        exit(1);
    }

    GLFWwindow* window = createWindow(WIDTH, HEIGHT, "Renderer Test App");

    gbg::RendererContext context = gbg::glfwCreateRendererContext(
        window, gbg::validationLayers, enableValidationLayers,
        gbg::deviceExtensions);

    gbg::SceneRenderer renderer(context);

    auto sc = std::make_shared<gbg::Scene>();

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

    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    mt.setShader(shh, sh);

    gbg::objLoader(arguments[1], sc.get(), sc->root, mth);

    renderer.setScene(sc);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // draw ui and modify scene
        // i will end up with a component system...

        float time = glfwGetTime();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            auto& st_mg = sc->getSceneTreeManager();
            gbg::SceneTreeNode& root = st_mg.get(sc->root);
            if (root.childH) {
                gbg::SceneTreeNode& modeln = st_mg.get(root.childH);
                float delta = time - glfwGetTime();
                time = glfwGetTime();
                modeln.transform = glm::rotate(modeln.transform, delta,
                                               glm::vec3(0.f, 1.f, 0.f));
            }
        }
        renderer.drawFrame();
    }
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
