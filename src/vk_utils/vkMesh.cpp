#include "vkMesh.hh"

#include <string.h>
#include <vulkan/vulkan_core.h>

#include <list>

#include "Logger.hpp"
#include "Mesh.hpp"
#include "glm/glm.hpp"
#include "vkBuffer.hh"

namespace gbg {
vkAttribute::vkAttribute(vkDevice device, uint attrib_id, size_t size,
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
    LOG("Creating attrib!")

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
vkAttribute::getAttributeDescriptions() const {
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

vkBuffer createIndexBuffer(vkDevice device,
                           const std::vector<std::list<uint>>& faces) {
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
    VkDeviceSize size = indices.size() * sizeof(indices[0]);
    LOG("Index Buffer Size: ")
    LOG_VAR(size)

    gbg::vkBuffer stagingBuffer =
        gbg::createBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device.ldevice, stagingBuffer.memory, 0, size, 0, &data);
    memcpy(data, indices.data(), size);
    vkUnmapMemory(device.ldevice, stagingBuffer.memory);

    vkBuffer indexBuffer = gbg::createBuffer(
        device, size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    copyBuffer(device, stagingBuffer, indexBuffer);
    vkDestroyBuffer(device.ldevice, stagingBuffer.buffer, nullptr);
    vkFreeMemory(device.ldevice, stagingBuffer.memory, nullptr);

    return indexBuffer;
}

void destroyMesh(const vkDevice& device, const vkMesh& mesh) {
    destroyBuffer(device, mesh.indexBuffer);
    for (const auto& attrb : mesh.vertexAttributes) {
        destroyBuffer(device, attrb.buffer);
    }
}
}  // namespace gbg
