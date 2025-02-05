
#include "resourceLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
namespace gbg {

TextureResource loadTexture(std::string texture_path) {
    TextureResource res;
    res.pixels = stbi_load(texture_path.c_str(), &res.width, &res.height,
                           &res.channels, STBI_rgb_alpha);
    if (!res.pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    return res;
}

Model loadModel(std::string model_path) {
    Model model;
    std::string warn, err;
    if (!tinyobj::LoadObj(&model.attrib, &model.shapes, &model.materials, &warn,
                          &err, model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    return model;
}
}  // namespace gbg
