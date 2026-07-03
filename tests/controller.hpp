
#include "AppData.hpp"
#include "GLFW/glfw3.h"
#include "glm/vec3.hpp"
#include "imgui.h"


inline glm::vec3 getOffset(GLFWwindow* window) {
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
    return offset;
}


inline void changeAppState(AppData& app, GLFWwindow* window) {
    if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Escape)) {
        if (not app.ui_mode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            app.ui_mode = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            app.ui_mode = false;
        }
    }
}
