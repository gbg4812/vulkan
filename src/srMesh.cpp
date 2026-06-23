#include "srMesh.hh"

#include <string.h>
#include <vulkan/vulkan_core.h>

#include <list>
#include <vector>

#include "Mesh.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/glm.hpp"
#include "vk_utils/Logger.hpp"
#include "vk_utils/vkBuffer.hh"

namespace gbg {
srAttribute::srAttribute(vkDevice device, uint attrib_id, size_t size,
                         AttributeTypes type, void* data)
    : attrib_id(attrib_id), type(type) {
    switch (type) {
        case gbg::AttributeTypes::VEC3_ATTR:
            size *= sizeof(glm::vec3);
            break;
        case FLOAT_ATTR:
            size *= sizeof(float);
            break;
        case VEC2_ATTR:
            size *= sizeof(glm::vec2);
            break;
    }

    this->size = size;

    VkDeviceSize dsize = size;

    gbg::vkBuffer stagingBuffer =
        gbg::createBuffer(device, dsize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* sdata;
    vkMapMemory(device.ldevice, stagingBuffer.memory, 0, dsize, 0, &sdata);
    memcpy(sdata, data, size);
    vkUnmapMemory(device.ldevice, stagingBuffer.memory);

    buffer = gbg::createBuffer(
        device, dsize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    copyBuffer(device, stagingBuffer, buffer);
    vkDestroyBuffer(device.ldevice, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device.ldevice, stagingBuffer.memory, nullptr);
}

std::pair<VkVertexInputBindingDescription, VkVertexInputAttributeDescription>
srAttribute::getAttributeDescriptions() const {
    VkVertexInputBindingDescription description{};
    VkVertexInputAttributeDescription attributeDescription{};
    switch (type) {
        case gbg::AttributeTypes::FLOAT_ATTR:
            description.stride = sizeof(float);
            attributeDescription.format = VK_FORMAT_R32_SFLOAT;
            break;
        case gbg::AttributeTypes::VEC2_ATTR:
            description.stride = sizeof(glm::vec2);
            attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
            break;
        case gbg::AttributeTypes::VEC3_ATTR:
            description.stride = sizeof(glm::vec3);
            attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
            break;
    }

    description.binding = attrib_id;
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    attributeDescription.location = attrib_id;
    attributeDescription.binding = attrib_id;
    attributeDescription.offset = 0;
    return {description, attributeDescription};
}

std::vector<uint32_t> createIndexBuffer(
    vkDevice device, const std::vector<std::list<uint>>& faces) {
    std::vector<uint32_t> indices;
    for (const auto& face : faces) {
        if (not face.empty()) {
            auto it = ++face.begin();
            while (it != --face.end()) {
                indices.push_back(*face.begin());
                indices.push_back(*it);
                indices.push_back(*++it);
            }
        }
    }
    return indices;
}

glm::vec3 computeTangent(glm::vec3 p1, glm::vec2 uv1, glm::vec3 p2,
                         glm::vec2 uv2, glm::vec3 p3, glm::vec2 uv3) {
    glm::vec3 edge1 = p2 - p1;
    glm::vec3 edge2 = p3 - p1;
    glm::vec2 duv1 = uv2 - uv1;
    glm::vec2 duv2 = uv3 - uv1;

    float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

    glm::vec3 t;
    t.x = f * (duv2.y * edge1.x - duv1.y * edge2.x);
    t.y = f * (duv2.y * edge1.y - duv1.y * edge2.y);
    t.z = f * (duv2.y * edge1.z - duv1.y * edge2.z);

    return t;
}

std::vector<glm::vec3> createTangentBuffer(
    vkDevice device, const std::vector<glm::vec3>& pos,
    const std::vector<glm::vec2> tex_coord,
    const std::vector<uint32_t> indices) {
    std::vector<glm::vec3> tangents(pos.size(), glm::vec3(0.0f));
    for (int i = 0; i < indices.size(); i += 3) {
        uint32_t i1 = indices[i];
        uint32_t i2 = indices[i + 1];
        uint32_t i3 = indices[i + 2];

        glm::vec3 tangent = computeTangent(pos[i1], tex_coord[i1], pos[i2],
                                              tex_coord[i2], pos[i3], tex_coord[i3]); 
        tangents[i1] += tangent;
        tangents[i2] += tangent;
        tangents[i3] += tangent;
    }

    for(auto& tan : tangents) {
        tan = normalize(tan);
    }
    
    return tangents;
}

void destroyMesh(const vkDevice& device, const srMesh& mesh) {
    destroyBuffer(device, mesh.indexBuffer);
    for (const auto& attrb : mesh.vertexAttributes) {
        destroyBuffer(device, attrb.buffer);
    }
}
}  // namespace gbg
