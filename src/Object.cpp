#include "Object.h"

#include "Logger.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>

Object::Object(std::string modelPath) : modelMat(1.0f) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          modelPath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
    LOG(vertices.size())
    LOG(indices.size())
}

const Vertex* Object::getVertexData() const { return vertices.data(); }
const uint32_t* Object::getIndexData() const { return indices.data(); }

const glm::mat4* Object::getModelMatrix() const { return &modelMat; }

uint32_t Object::getVertexSize() const {
    return static_cast<uint32_t>(vertices.size());
}
uint32_t Object::getIndexSize() const {
    return static_cast<uint32_t>(indices.size());
}

void Object::setTransform(glm::mat4 model) { modelMat = model; }
void Object::transform(glm::mat4 transform) { modelMat = transform * modelMat; }
