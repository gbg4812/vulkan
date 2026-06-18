
#include <glm/ext/vector_float3.hpp>

#include "Light.hpp"
#include "Resource.hpp"
namespace gbg {

// losing space
struct vkLight {
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 position;
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

};  // namespace gbg
