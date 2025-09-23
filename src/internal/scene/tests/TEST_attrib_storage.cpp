#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include "AttributeStorage.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Starting testing" << std::endl;
    typedef std::variant<std::vector<glm::vec3>, std::vector<glm::vec2>,
                         std::vector<float>, std::vector<int>>
        attrib_var_t;
    gbg::AttributeStorage<attrib_var_t> attrs;

    auto uv = attrs.getAttribute<glm::vec2>("uv");
    auto pos = attrs.getAttribute<glm::vec3>("pos");

    attrs.resize(10);

    float i = 0;
    for (auto& p : pos) {
        std::cout << glm::to_string(p) << std::endl;
        std::cout << i << std::endl;
        p.x = i;
        i++;
    }

    auto pos2 = attrs.getAttribute<glm::vec3>("pos");

    return 0;
}
