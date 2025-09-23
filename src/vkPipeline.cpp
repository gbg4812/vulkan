#include "vkPipeline.hh"

#include "glm/glm.hpp"

namespace gbg {
vkVertexInputDescription getVertexVector3InputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(glm::vec3);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.attrib_desc.offset = 0;
    return desc;
}
vkVertexInputDescription getVertexVector2InputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(glm::vec2);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32G32_SFLOAT;  // vec2 = 2 floats
    desc.attrib_desc.offset = 0;
    return desc;
}

vkVertexInputDescription getVertexFloatInputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(float);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32_SFLOAT;  // single float
    desc.attrib_desc.offset = 0;
    return desc;
}
}  // namespace gbg
