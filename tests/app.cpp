#include <vulkan/vulkan_core.h>

#include <iostream>
#include <memory>
#include <ostream>
#include <span>

#include "GlfwCreateRendererContext.hpp"
#include "RendererContext.hpp"
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
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "loaders/objLoader.hpp"
#include "loaders/texLoader.hpp"

const std::string TEXTURE_PATH =
    "data/textures/plank_texture/raw_plank_wall_diff_1k.png";

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
}

GLFWwindow* createWindow(int width, int height, std::string name) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window =
        glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForVulkan(window, true);

    return window;
}

int main(int argc, char* argv[]) {
    std::span arguments(argv, argc);

    if (arguments.size() < 2) {
        std::cout << "Usage: app obj-file-name" << std::endl;
        exit(1);
    }

    bool ui_mode = false;

    GLFWwindow* window = createWindow(WIDTH, HEIGHT, "Renderer Test App");
    glfwMaximizeWindow(window);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gbg::RendererContext context = gbg::glfwCreateRendererContext(
        window, gbg::validationLayers, enableValidationLayers,
        gbg::deviceExtensions);

    gbg::SceneRenderer renderer(context);

    setupGlfwCallbacks(window, &renderer);

    auto sc = std::make_shared<gbg::Scene>();

    auto& mt_mg = sc->getMaterialManager();
    auto& sh_mg = sc->getShaderManager();

    auto& tx_mg = sc->getTextureManager();
    auto tx_h = tx_mg.create("DiffuseTexture");

    loadTexture(TEXTURE_PATH, sc.get(), tx_h); // loads texture

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::Material& mt = mt_mg.get(mth);

    gbg::ShaderHandle shh = sh_mg.create("DiffuseShader");
    gbg::Shader& sh = sh_mg.get(shh);

    size_t colorp = sh.addParameter(gbg::ParameterTypes::VEC3_PARM);  // color
    size_t texp = sh.addParameter(gbg::TEXTURE_PARM); // texture

    sh.addAttribute(0, gbg::AttributeTypes::VEC3_ATTR);               // pos
    sh.addAttribute(1, gbg::AttributeTypes::VEC3_ATTR);               // normal
    sh.addAttribute(2, gbg::AttributeTypes::VEC2_ATTR);               // texture

    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    mt.setShader(shh, sh);
    mt.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(
        colorp, glm::vec3(0.0f, 1.0f, 1.0f));
    mt.setParameterValue<gbg::TEXTURE_PARM>(
        texp, tx_h);

    auto& cm_mg = sc->getCameraManager();
    auto& st_mg = sc->getSceneTreeManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("CameraObject");

    st_mg.get(cm_nh).translation += glm::vec3{0.0f, 0.0f, 10.0f};

    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(sc->root, cm_nh);

    std::cout << arguments[1] << std::endl;
    gbg::objLoader(arguments[1], sc.get(), sc->root, mth);

    renderer.setScene(sc);
    renderer.setActiveCamera(cm_nh);

    double time = glfwGetTime();

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // draw ui and modify scene
        // i will end up with a component system...

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float delta = glfwGetTime() - time;
        time = glfwGetTime();

        gbg::SceneTreeNode& cam_node = st_mg.get(cm_nh);
        glm::vec3 offset{};
        if (not ui_mode) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                offset.z += -2.0f * delta;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                offset.z += 2.0f * delta;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                offset.x += -2.0f * delta;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                offset.x += 2.0f * delta;
            }
        }

        cam_node.localTranslate(offset);

        if (not ui_mode) {
            double xnew, ynew;
            glfwGetCursorPos(window, &xnew, &ynew);
            double xdelta = xnew - xpos;
            double ydelta = ynew - ypos;
            xpos = xnew;
            ypos = ynew;
            cam_node.rotation.y += -0.001f * (float)xdelta;
            cam_node.rotation.x += -0.001f * (float)ydelta;
        }

        renderer.drawFrame();

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape)) {
            if (not ui_mode) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ui_mode = true;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                ui_mode = false;
            }
        }
    }
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
