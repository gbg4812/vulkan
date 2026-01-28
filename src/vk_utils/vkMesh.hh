#pragma once
#include <vulkan/vulkan_core.h>

#include <list>
#include <vector>

#include "Mesh.hpp"
#include "vkBuffer.hh"
#include "vkDevice.hh"
namespace gbg {

struct vkAttribute {
   public:
    vkAttribute(vkDevice device, uint attrib_id, size_t size,
                AttributeTypes type, void* data);

    std::pair<VkVertexInputBindingDescription,
              VkVertexInputAttributeDescription>
    getAttributeDescriptions() const;

   public:
    vkBuffer buffer;
    int attrib_id;
    size_t size;
    AttributeTypes type;
};

struct vkMesh {
    std::vector<vkAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

vkBuffer createIndexBuffer(vkDevice device,
                           const std::vector<std::list<uint>>& faces);

void destroyMesh(const vkDevice& device, const vkMesh& mesh);

}  // namespace gbg
