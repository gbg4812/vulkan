#pragma once
#include <glm/ext/vector_float3.hpp>

#include "Light.hpp"
#include "Resource.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "macros.hpp"
namespace gbg {

// TODO: losing space
struct vkLight {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 position;
    alignas(16) glm::mat4 proj;
    int shadow_map = -1;
};

struct srLight : public Resource<LightHandle> {
    RESOURCE_CONSTR(srLight)
    int light_index;
};

RELATED_RESOURCE_MANAGER(srLight, LightHandle);

};  // namespace gbg
