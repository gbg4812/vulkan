#include <vulkan/vulkan_core.h>
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
    glfwMaximizeWindow(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    size_t colorp = sh.addParameter(gbg::ParameterTypes::VEC3_PARM);     // color
    sh.addAttribute(0, gbg::AttributeTypes::VEC3_ATTR);  // pos
    sh.addAttribute(1, gbg::AttributeTypes::VEC3_ATTR);  // normal
    sh.addAttribute(2, gbg::AttributeTypes::VEC2_ATTR);  // texture

    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    mt.setShader(shh, sh);
    mt.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(colorp, glm::vec3(0.0f, 1.0f, 1.0f));

    auto& cm_mg = sc->getCameraManager();
    auto& st_mg = sc->getSceneTreeManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("CameraObject");

    st_mg.get(cm_nh).transform = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 10.0f});

    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(sc->root, cm_nh);


    gbg::objLoader(arguments[1], sc.get(), sc->root, mth);

    renderer.setScene(sc);
    renderer.setActiveCamera(cm_nh);

    double time = glfwGetTime();
    double xpos = 0.0, ypos = 0.0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // draw ui and modify scene
        // i will end up with a component system...

        float delta =  glfwGetTime() - time;
        time = glfwGetTime();

        gbg::SceneTreeNode& cam_node = st_mg.get(cm_nh);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cam_node.transform = glm::translate(cam_node.transform, {0.0, 0.0, -2.0f*delta});
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cam_node.transform = glm::translate(cam_node.transform, {0.0, 0.0, 2.0f*delta});
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cam_node.transform = glm::translate(cam_node.transform, {-2.0f*delta, 0.0, 0.0});
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cam_node.transform = glm::translate(cam_node.transform, {2.0f*delta, 0.0, 0.0 });
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        double xnew, ynew;
        glfwGetCursorPos(window, &xnew, &ynew);
        float xdelta = xnew - xpos;
        float ydelta = ynew - ypos;
        cam_node.transform = cam_node.transform * glm::rotate(glm::mat4(1.0f), -0.001f*xdelta, {0.0, 1.0, 0.0}) ;
        cam_node.transform = cam_node.transform * glm::rotate(glm::mat4(1.0f), -0.001f*ydelta, {1.0, 0.0, 0.0}) ;
        xpos = xnew;
        ypos = ynew;

        renderer.drawFrame();
    }
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
