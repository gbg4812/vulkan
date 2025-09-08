#pragma once
#include <vulkan/vulkan_core.h>

#include <list>
#include <memory>
#include <vector>

#include "Shader.hpp"
#include "vkBuffer.hh"
#include "vkDevice.hh"
namespace gbg {

struct vkAttribute {
   public:
    vkAttribute(vkDevice device, int attrib_id, size_t size, void* data);

   public:
    vkBuffer buffer;
    int attrib_id;
    size_t size;
};

struct vkMesh {
    std::vector<std::shared_ptr<gbg::vkAttribute>> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

vkBuffer createIndexBuffer(vkDevice device, std::vector<std::list<int>>& faces);

typedef struct vkVertexInputDescription {
    VkVertexInputAttributeDescription attrib_desc;
    VkVertexInputBindingDescription binding_desc;
} vkVertexInputDescription;

vkVertexInputDescription getInputDescription(int attrib_id, InputType type);

void destroyMesh(const vkDevice& device, const vkMesh& mesh);
}  // namespace gbg
