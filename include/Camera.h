#define GLM_FORCE_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma once

struct VPMatrices {
    // Vulkan requires us to align the descriptor data. If it is a scalar to N
    // (4 bytes given 32 bit floats or ints) If it is a vec2 to 2N and if it is
    // a vec4 to 4N. The alignas operator does this for us!
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class Camera {
    void updateCamera(double mousex, double mousey, bool front, bool back,
                      bool right, bool left) {
        transforms.view = glm::translate(
            transforms.view,
            glm::vec3(front * sensiblility + back * -sensiblility,
                      right * sensiblility + left * -sensiblility, 0));
    }

   private:
    VPMatrices transforms;
    double sensiblility;
};
