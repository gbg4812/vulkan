#include "vkMesh.hh"

#include <string.h>
#include <vulkan/vulkan_core.h>

#include <list>

#include "Logger.hpp"
#include "glm/glm.hpp"
#include "vkBuffer.hh"

namespace gbg {
vkAttribute::vkAttribute(vkDevice device, int attrib_id, size_t element_size,
                         size_t element_count, void* data)
    : attrib_id(attrib_id), size(element_count * element_size) {
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
vkVector3Attribute::vkVector3Attribute(vkDevice device, int attrib_id,
                                       size_t element_size,
                                       size_t element_count, void* data)
    : vkAttribute(device, attrib_id, element_size, element_count, data) {}

VkVertexInputBindingDescription vkVector3Attribute::getBindingDesc() {
    VkVertexInputBindingDescription description{};
    description.binding = attrib_id;
    description.stride = sizeof(glm::vec3);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
}
VkVertexInputAttributeDescription vkVector3Attribute::getAttribDesc() {
    VkVertexInputAttributeDescription description;
    description.location = attrib_id;
    description.binding = attrib_id;
    description.format = VK_FORMAT_R32G32B32_SFLOAT;
    description.offset = 0;
    return description;
}

vkBuffer createIndexBuffer(vkDevice device,
                           std::vector<std::list<int>>& faces) {
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
        destroyBuffer(device, attrb->buffer);
    }
}
}  // namespace gbg
