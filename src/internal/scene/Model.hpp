#pragma once
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "Shader.hpp"
#include "gbg_traits.hpp"
#include "glm/glm.hpp"
namespace gbg {

typedef std::variant<std::shared_ptr<std::vector<float>>,
                     std::shared_ptr<std::vector<glm::vec2>>,
                     std::shared_ptr<std::vector<glm::vec3>>>
    attr_variant_t;

struct PushDefaultElement {
    PushDefaultElement() {
        float_default = 0;
        vec2_default = glm::vec2(0.0f);
        vec3_default = glm::vec3(0.0f);
    }
    void operator()(std::shared_ptr<std::vector<float>> vec) {
        vec->push_back(float_default);
    }
    void operator()(std::shared_ptr<std::vector<glm::vec2>> vec) {
        vec->push_back(vec2_default);
    }
    void operator()(std::shared_ptr<std::vector<glm::vec3>> vec) {
        vec->push_back(vec3_default);
    }

    float float_default;
    glm::vec2 vec2_default;
    glm::vec3 vec3_default;
};

class Mesh {
   public:
    Mesh() : _vertex_cnt(0) {
        _positions = std::make_shared<std::vector<glm::vec3>>(0);
        _faces = std::make_shared<std::vector<std::list<int>>>(0);
    }

    template <AttributeType I>
    std::variant_alternative_t<to_underlying(I), attr_variant_t>
    createAttribute(const std::string& name) {
        if (name == "position" or _attributes.contains(name)) {
            return nullptr;
        }

        auto new_attrib_ptr =
            std::make_shared<typename std::variant_alternative_t<
                to_underlying(I), attr_variant_t>::element_type>(_vertex_cnt);

        _attributes[name] = new_attrib_ptr;

        return new_attrib_ptr;
    }

    template <AttributeType I>
    std::variant_alternative_t<to_underlying(I), attr_variant_t> getAttribute(
        const std::string& name) {
        if (name == "position" and I == AttributeType::Vector3) {
            return _positions;
        }
        auto it = _attributes.find(name);
        if (it == _attributes.end()) {
            return nullptr;
        }

        if (auto ptr = std::get_if<I>(&_attributes[name])) {
            return *ptr;
        }

        return nullptr;
    }

    // TODO: Produces crash
    int addVertex(glm::vec3 position) {
        _positions->push_back(position);
        _vertex_cnt++;

        for (auto it = _attributes.begin(); it != _attributes.end(); it++) {
            std::visit(PushDefaultElement{}, it->second);
        }

        return _positions->size() - 2;
    }

    template <AttributeType I>
    bool setAttributeValue(
        const std::string& name, int indice,
        std::variant_alternative_t<to_underlying(I), attr_variant_t> value) {
        if (name == "position" and I == AttributeType::Vector3) {
            (*_positions)[indice] = value;
            return true;
        }
        auto it = _attributes.find(name);
        if (it == _attributes.end()) {
            return false;
        }

        if (auto ptr = std::get_if<I>(&_attributes[name])) {
            (*ptr)[indice] = value;
            return true;
        }

        return false;
    }

    int addFace(std::list<int> face) {
        _faces->push_back(face);
        return _faces->size() - 2;
    }

    std::shared_ptr<std::vector<std::list<int>>> getFaces() { return _faces; };
    std::shared_ptr<std::vector<glm::vec3>> getPositions() {
        return _positions;
    }

   private:
    std::map<std::string, attr_variant_t> _attributes;
    std::shared_ptr<std::vector<std::list<int>>> _faces;
    std::shared_ptr<std::vector<glm::vec3>> _positions;
    size_t _vertex_cnt;
};

class Model {
   public:
    Model(std::shared_ptr<Material> mat, std::shared_ptr<Mesh> mesh)
        : _mat(mat), _mesh(mesh) {}

    void setMaterial(std::shared_ptr<Material> mat) { _mat = mat; }
    void setMesh(std::shared_ptr<Mesh> mesh) { _mesh = mesh; }

    std::shared_ptr<Material> getMat() { return _mat; }
    std::shared_ptr<Mesh> getMesh() { return _mesh; }

   private:
    std::shared_ptr<Material> _mat;
    std::shared_ptr<Mesh> _mesh;
};

}  // namespace gbg
