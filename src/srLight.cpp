#include "srLight.hpp"

#include <print>
#include <utility>

#include "Light.hpp"
#include "gbg_traits.hpp"
#include "geoc/frustum_utils.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

namespace gbg {
// add constant first size (with an offset?)
std::vector<vkLight> computeDirectionalLights(const Light& light, size_t N,
                                              const Camera& cam,
                                              glm::mat4 cam_t,
                                              glm::mat4 light_t, float aspect) {
    std::vector<vkLight> res(N);
    glm::mat4 light_view = glm::inverse(light_t);
    float fr_len = cam.zfar - cam.znear;
    for (int i = 0; i < N; i++) {
        float near = cam.znear + (1. / glm::pow(2, N - i)) * fr_len;
        if (i == 0) near = cam.znear;
        float far = cam.znear + (1. / glm::pow(2, N - (i + 1))) * fr_len;
        res[i].direction = light_t * glm::vec4(0, 0, -1, 0);
        res[i].color = light.color;
        res[i].intensity = light.intensity;
        res[i].type = to_underlying(LightType::DIRECTIONAL);
        res[i].position = glm::vec3(cam.znear, cam.zfar, near);

        // std::println("near {}, far {}", near, far);

        auto pts = getFrustumVolume(glm::radians(cam.fov), aspect, near, far);
        for (auto& pt : pts) {
            pt = cam_t * glm::vec4(pt, 1.0f);
            pt = light_view * glm::vec4(pt, 0.0f);
        }
        auto bbox = boundingBox3D(pts);

        float side_size = glm::max((bbox.second.x - bbox.first.x),
                                   (bbox.second.y - bbox.first.y));

        res[i].proj = glm::orthoRH_ZO(bbox.first.x, bbox.first.x + side_size,
                                      bbox.first.y + side_size, bbox.first.y,
                                      -bbox.second.z, -bbox.first.z) *
                      glm::mat4(glm::mat3(light_view));
    }

    return res;
}
}  // namespace gbg
