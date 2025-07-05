#include <iostream>
#include <vector>

#include "../Model.hpp"

int main(int argc, char* argv[]) {
    std::vector<float> floatvec = {1, 1, 1, 2, 2, 2, 3, 3, 3};
    gbg::Vec3AttributeData data(floatvec);
    for (const glm::vec3& el : data._data) {
        std::cout << el.x << el.y << el.z << std::endl;
    }
    return 0;
}
