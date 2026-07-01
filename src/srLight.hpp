
#include <glm/ext/vector_float3.hpp>

#include "Light.hpp"
#include "Resource.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "macros.hpp"
namespace gbg {

// losing space
struct vkLight {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 position;
    alignas(16) glm::mat4 proj;
};

struct srLight : public Resource {
    srLight() : Resource() {}
    srLight(std::string name, uint32_t rid) : Resource(name, rid) {}
    int light_index;
};

struct srLightHandle : public ResourceHandle {
    srLightHandle() : ResourceHandle(){};
    srLightHandle(uint32_t rid, size_t index) : ResourceHandle(rid, index){};
};

RESOURCE_MANAGER(srLight);

};  // namespace gbg
