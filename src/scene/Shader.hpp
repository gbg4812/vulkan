
#include <memory>
#include <string_view>
#include <vector>
namespace gbg {

enum InputType {
    TEXTURE,
};

struct ShaderInput {
    std::string_view name;
    InputType type;
};

struct InputData {
    InputType type;

   protected:
    InputData();
};

struct TextureInputData : InputData {
    TextureInputData() { type = TEXTURE; }
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int channels;
};

struct Shader {
    std::string_view vert_shader;
    std::string_view frag_shader;
    std::vector<ShaderInput> inputs;
};

struct Material {
    int id;
    std::shared_ptr<Shader> shader;
    std::vector<InputData> resources;
};

}  // namespace gbg
