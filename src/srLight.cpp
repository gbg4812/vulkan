#include "srLight.hpp"

#include <utility>

#include "Light.hpp"
#include "gbg_traits.hpp"
#include "geoc/frustum_utils.hpp"
#include "glm/exponential.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

namespace gbg {
std::array<vkLight, 4> computeDirectionalLights(const Light& light,
                                                const Camera& cam,
                                                glm::mat4 cam_t,
                                                glm::mat4 light_t,
                                                float aspect) {
    std::array<vkLight, 4> res;
    glm::mat4 light_view = glm::inverse(light_t);
    float fr_len = cam.zfar - cam.znear;
    for (int i = 0; i < 4; i++) {
        float near = cam.znear + (i / 4.0f) * fr_len;
        float far = cam.znear + ((i + 1) / 4.0f) * fr_len;
        res[i].direction = light_t * glm::vec4(0, 0, -1, 0);
        res[i].color = light.color;
        res[i].intensity = light.intensity;
        res[i].type = to_underlying(LightType::DIRECTIONAL);
        res[i].position = glm::vec3(cam.znear, cam.zfar, near);

        auto pts = getFrustumVolume(glm::radians(cam.fov), aspect, near, far);
        for (auto& pt : pts) {
            pt = cam_t * glm::vec4(pt, 1.0f);
            pt = light_view * glm::vec4(pt, 0.0f);
        }
        auto bbox = boundingBox3D(pts);

        res[i].proj =
            glm::orthoRH_ZO(bbox.first.x, bbox.second.x, bbox.second.y,
                            bbox.first.y, bbox.first.z, bbox.second.z) *
            glm::mat4(glm::mat3(light_view));
    }

    return res;
}
}  // namespace gbg
