#pragma once
#include <tiny_obj_loader.h>

#include <memory>
#include <string>

#include "Logger.hpp"
#include "Model.hpp"
#include "Shader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
namespace gbg {

std::shared_ptr<Image> loadTexture(std::string texture_path) {
    auto res = std::make_shared<Image>();
    stbi_uc* data = stbi_load(texture_path.c_str(), &res->width, &res->height,
                              &res->channels, STBI_rgb_alpha);
    size_t data_size = 4 * res->width * res->height;

    if (!data) {
        throw std::runtime_error("failed to load texture image!");
    }

    res->pixels.assign(data, data + data_size);

    stbi_image_free(data);
    return res;
}

std::shared_ptr<Mesh> loadMesh(std::string model_path) {
    LOG("Loading mesh inside function!")
    std::string warn, err;
    tinyobj::attrib_t attribs;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    if (!tinyobj::LoadObj(&attribs, &shapes, &materials, &warn, &err,
                          model_path.c_str(), NULL, false)) {
        throw std::runtime_error(warn + err);
    }
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    auto color = mesh->createAttribute<AttributeType::Vector3>("color");
    auto uv = mesh->createAttribute<AttributeType::Vector2>("uv");

    LOG_VAR(shapes.size())

    for (const auto& shape : shapes) {
        LOG("Adding shape!")
        int indices_idx = 0;
        for (const auto& face_verts : shape.mesh.num_face_vertices) {
            std::list<int> face;
            // for every vert of the face
            for (int i = 0; i < face_verts; i++) {
                const auto& index = shape.mesh.indices[indices_idx];
                glm::vec3 pos = {
                    attribs.vertices[3 * index.vertex_index + 0],
                    attribs.vertices[3 * index.vertex_index + 1],
                    attribs.vertices[3 * index.vertex_index + 2],
                };

                glm::vec2 texCoord = {

                    attribs.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attribs.texcoords[2 * index.texcoord_index + 1],
                };

                glm::vec3 vcolor = {1.0f, 1.0f, 1.0f};

                // only ok because i know how it works
                int vert = mesh->addVertex(pos);
                (*color)[vert] = vcolor;
                (*uv)[vert] = texCoord;

                face.push_back(vert);
                indices_idx++;
            }

            mesh->addFace(face);
        }
    }

    return mesh;
}

}  // namespace gbg
