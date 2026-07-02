#pragma once
#include "glm/mat4x4.hpp"

namespace gbg {
struct PerObjectPushConstant {
    glm::mat4 model;
};
}
