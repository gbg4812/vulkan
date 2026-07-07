#include <nfd.h>
#include <vulkan/vulkan_core.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <variant>

#include "GlfwCreateRendererContext.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneTree.hpp"
#include "Texture.hpp"
#include "createObject.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "loaders/objLoader.hpp"
#include "materialPanel.hpp"
#include "sceneObjectPanel.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "controller.hpp"
#include "imgui.h"
#include "io_utils/watcher.hpp"
#include "shaderReflexion.hpp"

#define TRACY_ENABLE 1
#include "AppData.hpp"
#include "createWindow.hpp"
#include "tracy/Tracy.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main(int argc, char* argv[]) {
    ZoneScoped;

    GLFWwindow* window = createWindow(WIDTH, HEIGHT, "Renderer Test App");
    glfwMaximizeWindow(window);

    gbg::RendererContext context = gbg::glfwCreateRendererContext(
        window, gbg::validationLayers, enableValidationLayers,
        gbg::deviceExtensions);

    init_watch();
    
    AppData app(context);

    setupGlfwCallbacks(window, &app);

    NFD_Init();
    // fix
    std::setlocale(LC_NUMERIC, "C");

    // Camera
    auto& st_mg = app.scene.getSceneTreeManager();
    auto& cm_mg = app.scene.getCameraManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("DefaultCamera");
    st_mg.get(cm_nh).translation += glm::vec3{12.0f, 5.0f, -3.0f};
    st_mg.get(cm_nh).rotation += glm::vec3{-0.3f, 1.92f, 0.0f};
    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(app.scene.root, cm_nh);
    app.scene.active_camera = cm_nh;

    // Light
    gbg::LightHandle lh = app.scene.lh_mg.create("Light");
    gbg::SceneTreeHandle lh_nh = st_mg.create("DefaultLigth");
    st_mg.get(lh_nh).setResource(lh);
    st_mg.get(lh_nh).translation = {5, 2, -5};
    st_mg.get(lh_nh).rotation.y = 130;
    st_mg.prependChild(app.scene.root, lh_nh);

    app.renderer.setScene(&app.scene);

    for (auto shh : app.scene.sh_mg) {
        app.scene.sh_mg.get(shh).clearFlags();
    }

    for (auto mth : app.scene.mat_mg) {
        app.scene.mat_mg.get(mth).clearFlags();
    }

    for (auto txh : app.scene.tx_mg) {
        app.scene.tx_mg.get(txh).clearFlags();
    }

    for (auto msh : app.scene.ms_mg) {
        app.scene.ms_mg.get(msh).clearFlags();
    }

    double time = glfwGetTime();

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        poll_watchers();
        // draw ui and modify scene
        // i will end up with a component system...

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float delta = glfwGetTime() - time;
        time = glfwGetTime();

        if (ImGui::Begin(
                "Stats", NULL,
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar)) {
            int fps = 1. / delta;
            ImGui::Text("FPS: %d", fps);
            ImGui::End();
        }

        gbg::SceneTreeNode& cam_node = st_mg.get(cm_nh);
        if (not app.ui_mode) {
            glm::vec3 offset = getOffset(window);
            cam_node.translation += glm::mat3(st_mg.getGlobalTransform(cm_nh)) *
                                    offset * delta * 2.0f;  // vel 2
        } else {

            drawCreateObject(app.scene);

            if (ImGui::BeginTabBar("Properties")) {
                if (ImGui::BeginTabItem("Scene Objects")) {
                    for (auto snh : st_mg) {
                        auto& sn = st_mg.get(snh);
                        drawSceneObjectPanel(app.scene, sn);
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Materials")) {
                    int i = 0;
                    for (auto math : app.scene.mat_mg) {
                        auto& mat = app.scene.mat_mg.get(math);
                        drawMaterialPanel(app.scene, mat);
                        i++;
                    }

                    drawNewMaterial(app.scene);

                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Shaders")) {
                    drawShaderPannel(app.scene);
                    ImGui::EndTabItem();
                }
                

                ImGui::EndTabBar();
            }  // end tab bar
        }

        double xnew, ynew;
        glfwGetCursorPos(window, &xnew, &ynew);
        if (not app.ui_mode) {
            double xdelta = xnew - xpos;
            double ydelta = ynew - ypos;
            cam_node.rotation.y += -0.1f * (float)xdelta;
            cam_node.rotation.x += -0.1f * (float)ydelta;
        }
        xpos = xnew;
        ypos = ynew;

        app.renderer.drawFrame();

        for (auto shh : app.scene.sh_mg) {
            app.scene.sh_mg.get(shh).clearFlags();
        }

        for (auto mth : app.scene.mat_mg) {
            app.scene.mat_mg.get(mth).clearFlags();
        }

        for (auto txh : app.scene.tx_mg) {
            app.scene.tx_mg.get(txh).clearFlags();
        }

        for (auto msh : app.scene.ms_mg) {
            app.scene.ms_mg.get(msh).clearFlags();
        }

        changeAppState(app, window);

        FrameMark;

    }  // end loop
    app.renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
