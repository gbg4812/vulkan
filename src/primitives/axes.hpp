#include "Mesh.hpp"
namespace gbg {
inline void axes(gbg::Mesh& mesh) {
    mesh.createAttribute<AttributeTypes::VEC3_ATTR>(0);  // position
    mesh.addVertex(4);
    auto& attrib = mesh.getAttribute<AttributeTypes::VEC3_ATTR>(0);
    attrib[0] = {0., 0., 0.};
    attrib[1] = {1., 0., 0.};
    attrib[2] = {0., 1., 0.};
    attrib[3] = {0., 0., 1.};
}
}  // namespace gbg
