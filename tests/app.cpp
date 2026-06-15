#include <vulkan/vulkan_core.h>

#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <variant>

#include "GlfwCreateRendererContext.hpp"
#include "RendererContext.hpp"
#include "Resource.hpp"
#include "SceneTree.hpp"
#include "Texture.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "traits/traits.hpp"
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
#include "shaderReflexion.hpp"

#define TRACY_ENABLE 1
#include "tracy/Tracy.hpp"

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
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
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
    ZoneScoped;
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
    gbg::ShaderHandle shh = sh_mg.create("DefaultShader");
    gbg::Shader& sh = sh_mg.get(shh);

    sh.loadVertShaderCode("./data/shaders/vert.spv");
    sh.loadFragShaderCode("./data/shaders/frag.spv");

    gbg::initShader(shh, sc);

    // Material Creation
    auto& mt_mg = sc.getMaterialManager();

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::MaterialHandle gmth = mt_mg.create("GreenMaterial");
    gbg::MaterialHandle rmth = mt_mg.create("RedMaterial");
    gbg::Material& mt = mt_mg.get(mth);
    gbg::Material& gmt = mt_mg.get(gmth);
    gbg::Material& rmt = mt_mg.get(rmth);

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

    mt.setShader(shh, sh, tx_h);
    gmt.setShader(shh, sh, tx_h);
    rmt.setShader(shh, sh, tx_h);

    // Other entities
    auto& st_mg = sc.getSceneTreeManager();
    auto& cm_mg = sc.getCameraManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("CameraObject");

    st_mg.get(cm_nh).translation += glm::vec3{0.0f, 0.0f, 10.0f};

    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(sc.root, cm_nh);

    std::cout << "Loading::" << arguments[1] << std::endl;

    gbg::objLoader(arguments[1], &sc, sc.root, mth);

    renderer.setScene(&sc);

    for (auto shh : sh_mg) {
        sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::NEW);
    }

    for (auto mth : mt_mg) {
        auto& mt = mt_mg.get(mth);
        mt.unsetFlag(gbg::ResourceFlags::NEW);
    }

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
            for (auto snh : st_mg) {
                auto& sn = st_mg.get(snh);
                if (ImGui::CollapsingHeader(sn.getName().c_str())) {
                    ImGui::InputFloat3("Translation", (float*)&sn.translation);
                    ImGui::InputFloat3("Rotation", (float*)&sn.rotation);
                    ImGui::InputFloat3("Scale", (float*)&sn.scale);

                    std::visit(
                        gbg::overloads{
                            [&](gbg::ModelHandle handle) {
                                gbg::Model& model = sc.md_mg.get(handle);
                                if (ImGui::BeginCombo(
                                        "Material",
                                        mt_mg.get(model.getMaterial())
                                            .getName()
                                            .c_str())) {
                                    for (auto mth : mt_mg) {
                                        bool selected =
                                            model.getMaterial() == mth;
                                        if (ImGui::Selectable(mt_mg.get(mth)
                                                                  .getName()
                                                                  .c_str(),
                                                              selected)) {
                                            model.setMaterial(mth);
                                        }
                                    }
                                    ImGui::EndCombo();
                                }
                            },
                            [&](auto&& def) {

                            },
                        },
                        sn.getResourceH());
                }
            }

            int i = 0;
            for (auto math : mt_mg) {
                auto& mat = mt_mg.get(math);
                if (ImGui::CollapsingHeader(mat.getName().c_str())) {
                    mat.unsetFlag(gbg::ResourceFlags::DIRTY);

                    glm::vec3 col =
                        mat.getParameterValue<gbg::ParameterTypes::VEC3_PARM>(
                            0);
                    if (ImGui::ColorPicker3("Material Color", (float*)&col)) {
                        mat.setParameterValue<gbg::ParameterTypes::VEC3_PARM>(
                            0, col);
                        mat.setFlags(gbg::ResourceFlags::DIRTY);
                    }

                    for (auto [num, value] :
                         mat.getValues() | std::views::enumerate) {
                        if (const gbg::TextureHandle* h =
                                std::get_if<gbg::TextureHandle>(&value)) {
                            auto& tex = tx_mg.get(*h);
                            if (ImGui::BeginCombo(
                                    ("Texture" + std::to_string(num - 1))
                                        .c_str(),
                                    tex.getName().c_str())) {
                                // for every texture
                                for (auto texh : tx_mg) {
                                    auto& tex2 = tx_mg.get(texh);
                                    if (ImGui::Selectable(
                                            tex2.getName().c_str())) {
                                        mat.setParameterValue<
                                            gbg::ParameterTypes::TEXTURE_PARM>(
                                            num, texh);
                                        mat.setFlags(gbg::ResourceFlags::DIRTY);
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }
                }
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

        for (auto shh : sh_mg) {
            sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::DIRTY);
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape)) {
            if (not ui_mode) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ui_mode = true;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                ui_mode = false;
            }
        }

        FrameMark;

    }  // end loop
    renderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
