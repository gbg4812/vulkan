#include <memory>
#include <string>
#include <vector>

#include "fwd.hpp"
namespace gbg {

enum AttributeType {
    VEC3,
    VEC2,
};

struct MeshAttribute {
    MeshAttribute(std::string name, AttributeType type) {
        _name = name;
        _type = type;
    }
    std::string _name;
    AttributeType _type;
};

struct AttributeData {
    AttributeType type;

   protected:
    AttributeData();
};

struct Vec3AttributeData : AttributeData {
    Vec3AttributeData(const std::vector<glm::vec3>& data) {
        type = AttributeType ::VEC3;
        _data = data;
    };
    Vec3AttributeData(const std::vector<float>& data) {
        type = AttributeType ::VEC3;
        _data.assign(data.begin(), data.end());
    }
    std::vector<glm::vec3> _data;
};

struct Mesh {
    std::vector<std::weak_ptr<MeshAttribute>> attributes;
    std::vector<AttributeData> attrib_data;
};

struct Model {
    int materialID;
    std::weak_ptr<Mesh> mesh;
};

}  // namespace gbg
