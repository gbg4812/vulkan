#pragma once
/*
 * API
 * Given a fov, aspect, distance -> width, height
 * Given fov + aspect + near + far -> 8 point bounding volume // obs assumed in
 * (0,0,0) pointing towards -z Given points -> bounding box (max, min)
 * TODO
 */

#include <array>

#include "glm/common.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
namespace gbg {

std::array<glm::vec3, 8> getFrustumVolume(float fov, float aspect, float near,
                                          float far);

template <std::ranges::range R>
requires std::is_same_v<std::ranges::range_value_t<R>, glm::vec3>
std::pair<glm::vec3, glm::vec3> boundingBox3D(const R& points) {
    std::pair<glm::vec3, glm::vec3> minmax;

    for(const auto& pt : points) {
        minmax.first.x = glm::min(minmax.first.x, pt.x);
        minmax.first.y = glm::min(minmax.first.y, pt.y);
        minmax.first.z = glm::min(minmax.first.z, pt.z);
        
        minmax.second.x = glm::max(minmax.second.x, pt.x);
        minmax.second.y = glm::max(minmax.second.y, pt.y);
        minmax.second.z = glm::max(minmax.second.z, pt.z);
    }
    
    return minmax;
}


}  // namespace gbg
