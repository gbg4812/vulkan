#include <nfd.h>
#include <vulkan/vulkan_core.h>

#include <cstdlib>

#include "GlfwCreateRendererContext.hpp"
#include "Material.hpp"
#include "RendererContext.hpp"
#include "SceneTree.hpp"
#include "Texture.hpp"
#include "createObject.hpp"
#include "materialPanel.hpp"
#include "sceneObjectPanel.hpp"
#define GLFW_INCLUDE_VULKAN
#include "AppData.hpp"
#include "GLFW/glfw3.h"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"
#include "Shader.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "controller.hpp"
#include "createWindow.hpp"
#include "imgui.h"
#include "io_utils/watcher.hpp"

#define TRACY_ENABLE 1
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
    NFD_Init();

    AppData app(context);

    setupGlfwCallbacks(window, &app);

    std::setlocale(LC_NUMERIC, "C");

    app.renderer.setScene(&app.scene);

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

        if (not app.ui_mode) {
            updateCamera(app.scene, window, delta);
        } else {
            drawCreateObject(app.scene);

            if (ImGui::BeginTabBar("Properties")) {
                if (ImGui::BeginTabItem("Scene Objects")) {
                    for (auto snh : app.scene.st_mg) {
                        auto& sn = app.scene.st_mg.get(snh);
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
