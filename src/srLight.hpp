#pragma once
#include <glm/ext/vector_float3.hpp>

#include "Camera.hpp"
#include "Light.hpp"
#include "gbg_traits.hpp"
#include "glm/ext/matrix_float4x4.hpp"
namespace gbg {

// TODO: improve packing
struct vkLight {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 position;
    alignas(16) glm::mat4 proj;
    float intensity;
    int shadow_map = -1;
    int type = to_underlying(LightType::SPOT);
};

std::array<vkLight, 4> computeDirectionalLights(const Light& light,
                                                const Camera& cam,
                                                glm::mat4 cam_t,
                                                glm::mat4 light_view);

};  // namespace gbg
