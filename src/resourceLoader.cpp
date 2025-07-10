
#include "resourceLoader.h"

#include <memory>

#include "scene/Scene.hpp"
#include "scene/Shader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
namespace gbg {

Image loadTexture(Scene& scene, std::string texture_path) {
    Image res;
    stbi_uc* data = stbi_load(texture_path.c_str(), &res.width, &res.height,
                              &res.channels, STBI_rgb_alpha);
    size_t data_size = 4 * res.width * res.height;

    if (!data) {
        throw std::runtime_error("failed to load texture image!");
    }

    res.pixels.assign(data, data + data_size);

    stbi_image_free(data);
    return res;
}

void loadModel(Scene& scene, std::string model_path) {
    std::string warn, err;
    tinyobj::attrib_t attribs;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    if (!tinyobj::LoadObj(&attribs, &shapes, &materials, &warn, &err,
                          model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

    for (const auto& vert : attribs.vertices) {
    }

    scene.meshes.push_back(mesh);
    model.mesh = mesh;
}

}  // namespace gbg
