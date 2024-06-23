#include <vulkan/vulkan_core.h>

#include "ext/matrix_transform.hpp"
#include "fwd.hpp"
#include "trigonometric.hpp"
#define GLM_FORCE_IMPLEMENTATION
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#pragma once

struct ViewMatrixUBO {
    // Vulkan requires us to align the descriptor data. If it is a scalar to N
    // (4 bytes given 32 bit floats or ints) If it is a vec2 to 2N and if it is
    // a vec4 to 4N. The alignas operator does this for us!
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class Camera {
   public:
    Camera(int width, int height, int fov, float nearPlane, float farPlane,
           float sensiblility = 0.1f)
        : transforms(1.0f),
          width(width),
          height(height),
          fov(fov),
          nearPlane(nearPlane),
          farPlane(farPlane),
          sensiblility(sensiblility) {
        transforms = glm::scale(transforms, {1.0f, -1.0f, -1.0f});
    }

    const ViewMatrixUBO ubo() const {
        ViewMatrixUBO UBO;
        UBO.view = glm::inverse(transforms);
        UBO.proj =
            glm::perspective(glm::radians((float)fov),
                             (float)width / (float)height, nearPlane, farPlane);
        return UBO;
    }
    void updateCamera(double mousex, double mousey, bool front, bool back,
                      bool right, bool left) {
        transforms = glm::translate(
            transforms,
            glm::vec3(right * sensiblility + left * -sensiblility, 0,
                      front * -sensiblility + back * sensiblility));
        transforms = glm::rotate(transforms, (float)mousex, glm::vec3(0, 1, 0));
        transforms = glm::rotate(transforms, (float)mousey, glm::vec3(1, 0, 0));
    }

   private:
    glm::mat4 transforms;
    int width;
    int height;
    int fov;
    float nearPlane;
    float farPlane;
    double sensiblility;
};
