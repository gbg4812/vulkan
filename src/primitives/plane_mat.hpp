#include <filesystem>
#include <stdexcept>
#include <string>

#include "MaterialFunctions.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "shaderReflexion.hpp"

namespace gbg {

const glm::vec3 RED = {1.0, 0.0, 0.0};
const glm::vec3 GREEN = {0.0, 1.0, 0.0};
const glm::vec3 BLUE = {0.0, 0.0, 1.0};

inline void solidShader(gbg::Shader& sh) {
    std::string fileNameFg =
        std::filesystem::path(__FILE__).parent_path().append(
            "/plane_shader.frag");
    auto err = setShaderCode(sh, fileNameFg, ShaderTypes::FRAGMENT);
    if (not err.first) {
        throw std::runtime_error(err.second);
    }
    std::string fileNameVt =
        std::filesystem::path(__FILE__).parent_path().append(
            "/plane_shader.vert");
    err = setShaderCode(sh, fileNameVt, ShaderTypes::VERTEX);
    if (not err.first) {
        throw std::runtime_error(err.second);
    }
    reflectShader(sh);
}

inline MaterialHandle solidColorMaterial(gbg::ShaderHandle sh, gbg::Scene& sc,
                                         glm::vec3 color) {
    auto& mt = sc.mat_mg.create("NewMaterial");
    mt.setShader(sh);
    setParametersFromShader(sc, mt);
    mt.setParameterValue<ParameterTypes::VEC3>(0, color);
    return mt.getHandle();
}

}  // namespace gbg
