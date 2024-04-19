#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "Vertex.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>

#pragma once

class Object {
   private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 modelMat;

   public:
    Object(std::string modelPath);

    void setTransform(glm::mat4 model);
    void transform(glm::mat4 transform);

    const glm::mat4* getModelMatrix() const;
    const Vertex* getVertexData() const;
    const uint32_t* getIndexData() const;
    uint32_t getVertexSize() const;
    uint32_t getIndexSize() const;
};
