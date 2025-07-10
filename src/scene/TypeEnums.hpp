
#include <cstddef>
namespace gbg {

enum class AttributeType : size_t {
    Float = 0,
    Vector2,
    Vector3,
};

// enum must extend AttributeType
enum class InputType : size_t {
    Float = 0,
    Vector2,
    Vector3,
    Mat4x4,
    Texture,
};
}  // namespace gbg
