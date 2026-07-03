
#pragma once
#include <iostream>
#include "AppData.hpp"
#include "SceneRenderer.hpp"

#include "GLFW/glfw3.h"
#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app =
        reinterpret_cast<AppData*>(glfwGetWindowUserPointer(window));

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    app->renderer.resizeSwapchain(width, height);
}


void window_focus_callback(GLFWwindow* window, int focused) {
        auto app =
            reinterpret_cast<AppData*>(glfwGetWindowUserPointer(window));
    if (focused && not app->ui_mode) {
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
