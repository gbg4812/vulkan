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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gbg::RendererContext context = gbg::glfwCreateRendererContext(
        window, gbg::validationLayers, enableValidationLayers,
        gbg::deviceExtensions);

    gbg::SceneRenderer renderer(context);

    setupGlfwCallbacks(window, &renderer);

    gbg::Scene sc;

    auto& sh_mg = sc.getShaderManager();

    // Shader Creation
    gbg::ShaderHandle shh = sh_mg.create("DiffuseShader");
    gbg::Shader& sh = sh_mg.get(shh);

    size_t colorp = sh.addParameter(gbg::ParameterTypes::VEC3_PARM);  // color
    size_t texp = sh.addParameter(gbg::TEXTURE_PARM);                 // texture

    sh.addAttribute(0, gbg::AttributeTypes::VEC3_ATTR);  // pos
    sh.addAttribute(1, gbg::AttributeTypes::VEC3_ATTR);  // normal
    sh.addAttribute(2, gbg::AttributeTypes::VEC2_ATTR);  // texture

    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    // Material Creation
    auto& mt_mg = sc.getMaterialManager();

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::MaterialHandle gmth = mt_mg.create("GreenMaterial");
    gbg::MaterialHandle rmth = mt_mg.create("RedMaterial");
    gbg::Material& mt = mt_mg.get(mth);
    gbg::Material& gmt = mt_mg.get(gmth);
    gbg::Material& rmt = mt_mg.get(rmth);

    mt.setShader(shh, sh);
    gmt.setShader(shh, sh);
    rmt.setShader(shh, sh);

    mt.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(
        colorp, glm::vec3(0.0f, 0.0f, 1.0f));
    gmt.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(
        colorp, glm::vec3(0.0f, 1.0f, .0f));
    rmt.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(
        colorp, glm::vec3(1.0f, 0.0f, 0.0f));

    auto& tx_mg = sc.getTextureManager();
    auto tx_h = tx_mg.create("DiffuseTexture");
    auto gtx_h = tx_mg.create("StoneTex");
    auto btx_h = tx_mg.create("WoolTex");

    loadTexture("data/textures/plank_texture/raw_plank_wall_diff_1k.png", &sc,
                tx_h);  // loads texture
    loadTexture(
        "data/textures/plaster_stone_wall_02_1k/"
        "plaster_stone_wall_02_diff_1k.jpg",
        &sc,
        gtx_h);  // loads texture
    loadTexture("data/textures/wool_boucle_1k/wool_boucle_diff_1k.png", &sc,
                btx_h);  // loads texture

    mt.setParameterValue<gbg::TEXTURE_PARM>(texp, tx_h);
    gmt.setParameterValue<gbg::TEXTURE_PARM>(texp, gtx_h);
    rmt.setParameterValue<gbg::TEXTURE_PARM>(texp, btx_h);

    // Other entities
    auto& st_mg = sc.getSceneTreeManager();
    auto& cm_mg = sc.getCameraManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("CameraObject");

    st_mg.get(cm_nh).translation += glm::vec3{0.0f, 0.0f, 10.0f};

    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(sc.root, cm_nh);

    std::cout << arguments[1] << std::endl;

    gbg::objLoader(arguments[1], &sc, sc.root, mth);

    sc.getModelManager()
        .get(st_mg.get(st_mg.get(sc.root).childH)
                 .getResourceH<gbg::SceneObjectTypes::MODEL>())
        .setMaterial(rmth);

    sc.getModelManager()
        .get(st_mg.get(st_mg.get(st_mg.get(sc.root).childH).nextH)
                 .getResourceH<gbg::SceneObjectTypes::MODEL>())
        .setMaterial(gmth);

    renderer.setScene(&sc);
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
        } else {
            ImGui::BeginGroup();
            int i = 0;
            for (auto& sn : st_mg) {
                ImGui::PushID(i);
                ImGui::Text(sn.getName().c_str());
                ImGui::InputFloat3("Translation", (float*)&sn.translation);
                ImGui::InputFloat3("Rotation", (float*)&sn.rotation);
                ImGui::InputFloat3("Scale", (float*)&sn.scale);
                ImGui::PopID();
                i++;
            }
            ImGui::EndGroup();
        }

        cam_node.localTranslate(offset);

        double xnew, ynew;
        glfwGetCursorPos(window, &xnew, &ynew);
        if (not ui_mode) {
            double xdelta = xnew - xpos;
            double ydelta = ynew - ypos;
            cam_node.rotation.y += -0.001f * (float)xdelta;
            cam_node.rotation.x += -0.001f * (float)ydelta;
        }
        xpos = xnew;
        ypos = ynew;

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
