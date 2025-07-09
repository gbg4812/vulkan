#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "fwd.hpp"
namespace gbg {

enum AttributeType {
    Vector3,
    Vector2,
    Float,
};

class Attribute {
   public:
    AttributeType type;
    void* data;
};

class Mesh {
   private:
    std::map<std::string, std::shared_ptr<Attribute>> attrib_data;

   public:
    std::vector<glm::vec3>* getVec3Attribute(std::string name) {
        auto it = attrib_data.find(name);
        if (it->second->type != AttributeType::Vector3 or
            it == attrib_data.end()) {
            return nullptr;
        }
        return reinterpret_cast<std::vector<glm::vec3>*>(
            attrib_data[name]->data);
    }
    std::vector<glm::vec2>* getVec2Attribute(std::string name) {
        auto it = attrib_data.find(name);
        if (it->second->type != AttributeType::Vector2 or
            it == attrib_data.end()) {
            return nullptr;
        }
        return reinterpret_cast<std::vector<glm::vec2>*>(
            attrib_data[name]->data);
    }
    std::vector<float>* getFloatAttribute(std::string name) {
        auto it = attrib_data.find(name);
        if (it->second->type != AttributeType::Vector2 or
            it == attrib_data.end()) {
            return nullptr;
        }
        return reinterpret_cast<std::vector<float>*>(attrib_data[name]->data);
    }
};

class Model {
    int materialID;
    std::weak_ptr<Mesh> mesh;
};

}  // namespace gbg
