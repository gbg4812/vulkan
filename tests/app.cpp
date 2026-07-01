#include <nfd.h>
#include <vulkan/vulkan_core.h>

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
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
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
#include "io_utils/watcher.hpp"
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
bool ui_mode = true;

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

void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused && not ui_mode) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void setupGlfwCallbacks(GLFWwindow* window, void* userPointer) {
    glfwSetWindowUserPointer(window, userPointer);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetWindowFocusCallback(window, window_focus_callback);
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
    ImGui::GetPlatformIO().Platform_LocaleDecimalPoint = '.';

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

    GLFWwindow* window = createWindow(WIDTH, HEIGHT, "Renderer Test App");
    glfwMaximizeWindow(window);

    gbg::RendererContext context = gbg::glfwCreateRendererContext(
        window, gbg::validationLayers, enableValidationLayers,
        gbg::deviceExtensions);

    gbg::SceneRenderer renderer(context);

    setupGlfwCallbacks(window, &renderer);

    init_watch();
    NFD_Init();

    gbg::Scene sc;

    auto& sh_mg = sc.getShaderManager();

    // Shader Creation
    gbg::ShaderHandle shh = sh_mg.create("DefaultShader");
    gbg::Shader& sh = sh_mg.get(shh);

    auto res =
        gbg::setShaderCode(sh, "./data/shaders/shader.vert", gbg::VERTEX);
    if (not res.first) {
        std::cout << res.second << std::endl;
    }
    res = gbg::setShaderCode(sh, "./data/shaders/shader.frag", gbg::FRAGMENT);
    if (not res.first) {
        std::cout << res.second << std::endl;
    }
    gbg::reflectShader(sh);

    // Material Creation
    auto& mt_mg = sc.getMaterialManager();

    gbg::MaterialHandle mth = mt_mg.create("DefaultMaterial");
    gbg::Material& mt = mt_mg.get(mth);

    auto& tx_mg = sc.getTextureManager();
    auto tx_h = tx_mg.create("DiffuseTexture");
    auto tx1_h = tx_mg.create("StoneTexture");

    loadTexture("data/textures/plank_texture/raw_plank_wall_diff_1k.png", &sc,
                tx_h);  // loads texture
    loadTexture("data/textures/plank_texture/raw_plank_wall_nor_gl_1k.png", &sc,
                tx1_h);           // loads texture
    tx_mg.get(tx1_h).raw = true;  // not srgb

    mt.setShader(shh, sh, tx_h);
    mt.setParameterValue<gbg::TEXTURE_PARM>(3, tx1_h);

    watch({"./data/shaders/shader.frag", "./data/shaders/shader.vert"},
          (uint32_t)WatchEvents::MODFY, [&]() {
              auto res = gbg::setShaderCode(sh, "./data/shaders/shader.vert",
                                            gbg::VERTEX);
              if (not res.first) {
                  std::cout << res.second << std::endl;
              } else {
                  std::cout << "Shader recompiled successfuly" << std::endl;
              }
              res = gbg::setShaderCode(sh, "./data/shaders/shader.frag",
                                       gbg::FRAGMENT);
              if (not res.first) {
                  std::cout << res.second << std::endl;
              } else {
                  std::cout << "Shader recompiled successfuly" << std::endl;
              }

              gbg::reflectShader(sh);

              for (gbg::MaterialHandle mh : sc.mat_mg) {
                  sc.mat_mg.get(mh).setShader(shh, sh, tx_h);
                  sc.mat_mg.get(mh).setFlags(gbg::ResourceFlags::DIRTY);
              }
              sh.setFlags(gbg::ResourceFlags::DIRTY);
          });

    // Camera
    auto& st_mg = sc.getSceneTreeManager();
    auto& cm_mg = sc.getCameraManager();
    gbg::CameraHandle camh = cm_mg.create("Camera");
    gbg::SceneTreeHandle cm_nh = st_mg.create("CameraObject");
    st_mg.get(cm_nh).translation += glm::vec3{12.0f, 5.0f, -3.0f};
    st_mg.get(cm_nh).rotation += glm::vec3{-0.3f, 1.92f, 0.0f};
    st_mg.get(cm_nh).setResource(camh);
    st_mg.prependChild(sc.root, cm_nh);
    sc.active_camera = cm_nh;

    // Light
    gbg::LightHandle lh1 = sc.lh_mg.create("Light1");
    gbg::LightHandle lh2 = sc.lh_mg.create("Light2");
    gbg::SceneTreeHandle lh_nh1 = st_mg.create("LightObject1");
    gbg::SceneTreeHandle lh_nh2 = st_mg.create("LightObject2");
    st_mg.get(lh_nh1).setResource(lh1);
    st_mg.get(lh_nh2).setResource(lh2);
    st_mg.get(lh_nh1).translation = {1, 2, -3};
    st_mg.get(lh_nh2).translation = {5, 2, -4};
    st_mg.prependChild(sc.root, lh_nh1);
    st_mg.prependChild(sc.root, lh_nh2);

    std::cout << "Loading::" << arguments[1] << std::endl;

    gbg::objLoader(arguments[1], &sc, sc.root, mth);
    std::cout << "Obj loaded" << std::endl;

    renderer.setScene(&sc);

    for (auto shh : sh_mg) {
        sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::NEW);
        sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::DIRTY);
    }

    for (auto mth : mt_mg) {
        auto& mt = mt_mg.get(mth);
        mt.unsetFlag(gbg::ResourceFlags::NEW);
        mt.unsetFlag(gbg::ResourceFlags::DIRTY);
    }

    for (auto txh : tx_mg) {
        auto& tx = tx_mg.get(txh);
        tx.unsetFlag(gbg::ResourceFlags::NEW);
        tx.unsetFlag(gbg::ResourceFlags::DIRTY);
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
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                offset.y += 2.0f * delta;
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                offset.y -= 2.0f * delta;
            }
        } else {
            if (ImGui::BeginTabBar("Properties")) {
                if (ImGui::BeginTabItem("Scene Objects")) {
                    for (auto snh : st_mg) {
                        auto& sn = st_mg.get(snh);
                        ImGui::PushID(sn.getRID());
                        if (ImGui::CollapsingHeader(sn.getName().c_str())) {
                            ImGui::InputFloat3("Translation",
                                               (float*)&sn.translation);
                            ImGui::InputFloat3("Rotation",
                                               (float*)&sn.rotation);
                            ImGui::InputFloat3("Scale", (float*)&sn.scale);

                            std::visit(
                                gbg::overloads{
                                    [&](gbg::ModelHandle handle) {
                                        gbg::Model& model =
                                            sc.md_mg.get(handle);
                                        if (ImGui::BeginCombo(
                                                "Material",
                                                mt_mg.get(model.getMaterial())
                                                    .getName()
                                                    .c_str())) {
                                            for (auto mth : mt_mg) {
                                                bool selected =
                                                    model.getMaterial() == mth;
                                                if (ImGui::Selectable(
                                                        mt_mg.get(mth)
                                                            .getName()
                                                            .c_str(),
                                                        selected)) {
                                                    model.setMaterial(mth);
                                                }
                                            }
                                            ImGui::EndCombo();
                                        }
                                    },
                                    [&](gbg::LightHandle handle) {
                                        gbg::Light& light =
                                            sc.lh_mg.get(handle);
                                        ImGui::ColorPicker3(
                                            "Light Color",
                                            (float*)&light.color);
                                    },
                                    [&](auto&& def) {

                                    },
                                },
                                sn.getResourceH());
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Materials")) {
                    int i = 0;
                    for (auto math : mt_mg) {
                        auto& mat = mt_mg.get(math);
                        if (ImGui::CollapsingHeader(mat.getName().c_str())) {
                            for (auto [num, value] :
                                 mat.getValues() | std::views::enumerate) {
                                if (const gbg::TextureHandle* h =
                                        std::get_if<gbg::TextureHandle>(
                                            &value)) {
                                    auto& tex = tx_mg.get(*h);
                                    if (ImGui::BeginCombo(
                                            ("Texture" +
                                             std::to_string(num - 1))
                                                .c_str(),
                                            tex.getName().c_str())) {
                                        // for every texture
                                        for (auto texh : tx_mg) {
                                            auto& tex2 = tx_mg.get(texh);
                                            if (ImGui::Selectable(
                                                    tex2.getName().c_str())) {
                                                mat.setParameterValue<
                                                    gbg::ParameterTypes::
                                                        TEXTURE_PARM>(num,
                                                                      texh);
                                                mat.setFlags(
                                                    gbg::ResourceFlags::DIRTY);
                                            }
                                        }

                                        ImGui::EndCombo();
                                    }

                                } else if (const glm::vec3* vec =
                                               std::get_if<glm::vec3>(&value)) {
                                    glm::vec3 col = *vec;
                                    if (ImGui::ColorPicker3(
                                            ("Parameter" + std::to_string(num))
                                                .c_str(),
                                            (float*)&col)) {
                                        mat.setParameterValue<
                                            gbg::ParameterTypes::VEC3_PARM>(
                                            num, col);
                                        mat.setFlags(gbg::ResourceFlags::DIRTY);
                                    }
                                } else if (const glm::vec2* vec =
                                               std::get_if<glm::vec2>(&value)) {
                                    glm::vec2 col = *vec;
                                    if (ImGui::InputFloat2(
                                            ("Parameter" + std::to_string(num))
                                                .c_str(),
                                            (float*)&col)) {
                                        mat.setParameterValue<
                                            gbg::ParameterTypes::VEC2_PARM>(
                                            num, col);
                                        mat.setFlags(gbg::ResourceFlags::DIRTY);
                                    }
                                } else if (const float* val =
                                               std::get_if<float>(&value)) {
                                    float f = *val;
                                    if (ImGui::InputFloat(
                                            ("Parameter" + std::to_string(num))
                                                .c_str(),
                                            &f)) {
                                        mat.setParameterValue<
                                            gbg::ParameterTypes::FLOAT_PARM>(
                                            num, f);
                                        mat.setFlags(gbg::ResourceFlags::DIRTY);
                                    }
                                }
                            }

                            static bool raw = false;

                            if (ImGui::Button("New Texture")) {
                                ImGui::OpenPopup("New Texture");
                                raw = false;
                            }

                            if (ImGui::BeginPopupModal(
                                    "New Texture", NULL,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
                                nfdu8char_t* outpath = nullptr;
                                static char buff[1024] = "";
                                static char name[64] = "";
                                if (ImGui::Button("Search")) {
                                    nfdopendialognargs_t args = {0};
                                    nfdresult_t res =
                                        NFD_OpenDialogU8_With(&outpath, &args);
                                    if (res == NFD_OKAY) {
                                        if (strlen(outpath) < sizeof(buff))
                                            strcpy(buff, outpath);
                                        NFD_FreePathU8(outpath);
                                    }
                                }

                                ImGui::InputText("File path", buff,
                                                 sizeof(buff));
                                ImGui::InputText("Name", name, sizeof(name));
                                ImGui::Checkbox("Raw", &raw);

                                if (ImGui::Button("Confirm")) {
                                    auto hand = tx_mg.create(name);
                                    loadTexture(buff, &sc, hand);
                                    tx_mg.get(hand).raw = raw;
                                    ImGui::CloseCurrentPopup();
                                }

                                ImGui::EndPopup();
                            }
                        }
                        i++;
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }  // end tab bar
        }

        cam_node.translation += glm::mat3(st_mg.getGlobalTransform(cm_nh)) * offset;

        double xnew, ynew;
        glfwGetCursorPos(window, &xnew, &ynew);
        if (not ui_mode) {
            double xdelta = xnew - xpos;
            double ydelta = ynew - ypos;
            cam_node.rotation.y += -0.1f * (float)xdelta;
            cam_node.rotation.x += -0.1f * (float)ydelta;
        }
        xpos = xnew;
        ypos = ynew;

        renderer.drawFrame();

        for (auto shh : sh_mg) {
            sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::NEW);
            sh_mg.get(shh).unsetFlag(gbg::ResourceFlags::DIRTY);
        }

        for (auto mth : mt_mg) {
            auto& mt = mt_mg.get(mth);
            mt.unsetFlag(gbg::ResourceFlags::NEW);
            mt.unsetFlag(gbg::ResourceFlags::DIRTY);
        }

        for (auto txh : tx_mg) {
            auto& tx = tx_mg.get(txh);
            tx.unsetFlag(gbg::ResourceFlags::NEW);
            tx.unsetFlag(gbg::ResourceFlags::DIRTY);
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
