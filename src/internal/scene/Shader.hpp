#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "TypeEnums.hpp"
#include "gbg_traits.hpp"
#include "glm/glm.hpp"

/*
 * Attributes and Inputs will be bound in the shader by alphabetical order and
 * starting at 0
 */

namespace gbg {

typedef std::filesystem::path path_t;

class Scene;

struct Image {
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int channels;
};

typedef std::variant<float, glm::vec2, glm::vec3, glm::mat4,
                     std::shared_ptr<Image>>
    input_variant_t;

class Shader {
   public:
    Shader(path_t vert_shader, path_t frag_shader)
        : _vert_shader(vert_shader), _frag_shader(frag_shader) {
        this->addInputAttribute("position", AttributeType::Vector3);
    }

    bool addInput(const std::string& name, InputType type) {
        auto it = _inputs.find(name);
        if (it != _inputs.end()) {
            return false;
        }

        _inputs.emplace(name, type);
        return true;
    };

    bool addInputAttribute(const std::string& name, AttributeType type) {
        auto it = _input_attributes.find(name);
        if (it != _input_attributes.end()) {
            return false;
        }

        _input_attributes.emplace(name, type);
        return true;
    }

   private:
    path_t _vert_shader;
    path_t _frag_shader;
    std::map<std::string, InputType> _inputs;
    std::map<std::string, AttributeType> _input_attributes;

    friend class Material;
};

class Material {
   public:
    Material(int shader) : _shader(shader) {}

    template <InputType I>
    bool setResource(
        const std::string& input_name,
        std::variant_alternative_t<to_underlying(I), input_variant_t>
            resource) {
        auto it = _shader->_inputs.find(input_name);
        if (it == _shader->_inputs.end()) {
            return false;
        }

        _resources[input_name] = resource;
        return true;
    }

    const std::map<std::string, input_variant_t>& getResources() const {
        return _resources;
    }

   private:
    int _shader;
    std::map<std::string, input_variant_t> _resources;
};

}  // namespace gbg
