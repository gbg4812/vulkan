
#ifndef GBG_RESLOD
#define GBG_RESLOD
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <string>
#include <string_view>
#include <vector>

namespace gbg {

struct Resource {
    enum Type {
        TEXTURE,
        INT,
        FLOAT,
    };

    Resource::Type type;
};

struct Shader {
    int id;
    std::string_view shaderPath;
    std::vector<Resource> bindings;
};

struct Material {
    int id;
    int shaderID;
    std::vector<Resource*> resources;
};

struct TextureResource : Resource {
    TextureResource() { type = TEXTURE; }
    ~TextureResource() { stbi_image_free(pixels); }
    stbi_uc* pixels;
    int width;
    int height;
    int channels;
};

struct IntResource : Resource {
    IntResource() { type = INT; }
    int value;
};

struct Model {
    int materialID;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

Model loadModel(std::string model_path);

TextureResource loadTexture(std::string texture_path);

}  // namespace gbg

#endif
