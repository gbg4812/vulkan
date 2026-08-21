
#include <X11/Xlib.h>

#include "AppData.hpp"
#include "GLFW/glfw3.h"
#include "imgui.h"

inline void updateCamera(AppData& app, GLFWwindow* window, float delta) {
    gbg::Scene& scene = app.scene;
    gbg::SceneTreeNode& cam_node = scene.st_mg.get(scene.active_camera);
    glm::vec3 offset = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        offset.z = -1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        offset.z = 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        offset.x = -1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        offset.x = 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        offset.y = 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        offset.y = -1.0f;
    }
    cam_node.translation +=
        glm::mat3(scene.st_mg.getGlobalTransform(scene.active_camera)) *
        offset * delta * 2.0f;  // vel 2

    double xnew, ynew;
    glfwGetCursorPos(window, &xnew, &ynew);
    double xdelta = xnew - app.cursor_pos.x;
    double ydelta = ynew - app.cursor_pos.y;
    cam_node.rotation.y += -0.1f * (float)xdelta;
    cam_node.rotation.x += -0.1f * (float)ydelta;
    app.cursor_pos.x = xnew;
    app.cursor_pos.y = ynew;
}

inline void changeAppState(AppData& app, GLFWwindow* window) {
    if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape)) {
        if (not app.ui_mode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            app.ui_mode = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPos(window, app.cursor_pos.x, app.cursor_pos.y);
            app.ui_mode = false;
        }
    }
}
