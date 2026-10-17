#include "frustum_utils.hpp"

#include <array>

#define GLM_FORCE_RADIANS
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
/*
 * API
 * Given a fov, aspect, distance -> width, height
 * Given fov + aspect + near + far -> 8 point bounding volume // obs assumed in
 * (0,0,0) pointing towards -z Given points -> bounding box (max, min)
 * TODO
 */

namespace gbg {

/**
 *
 * @param fov in radians
 */
std::array<glm::vec3, 8> getFrustumVolume(float fov, float aspect, float near,
                                          float far) {
    float a2 = fov / 2;
    float nh = glm::tan(a2) * near;
    float nw = nh * aspect;
    float fh = glm::tan(a2) * far;
    float fw = fh * aspect;

    std::array<glm::vec3, 8> res = {
        glm::vec3(-nw, nh, -near), glm::vec3(-nw, -nh, -near),
        glm::vec3(nw, -nh, -near), glm::vec3(nw, nh, -near),
        glm::vec3(-fw, fh, -far),  glm::vec3(-fw, -fh, -far),
        glm::vec3(fw, -fh, -far),  glm::vec3(fw, fh, -far)};
    return res;
}

}  // namespace gbg
