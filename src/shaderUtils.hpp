#pragma once
/**
 * (bool, string) compileShader(shader); // read the files compile and reflect
 * (create parameters, set defaults, create attributes) Use cases:
 * - when the shader code changes
 *
 * (bool, string) checkShaderCompatibility(mesh, shader); // check that the mesh
 * has the correct attributes
 * - to report errors and switch to default
 */

#include <expected>
#include <string>

#include "Shader.hpp"
namespace gbg {

void initShaderCompiler();

std::expected<std::string, std::string> compileShader(FileManager& f_m,
                                                      Shader& shader);
std::expected<void, std::string> checkShaderCompatibility(const Mesh& mesh,
                                                          const Shader& shader);
}  // namespace gbg
