#pragma once
#include <vulkan/vulkan_core.h>

#include <list>
#include <vector>

#include "Mesh.hpp"
#include "Resource.hpp"
#include "vk_utils/vkBuffer.hh"
#include "vk_utils/vkDevice.hh"
namespace gbg {

struct srAttribute {
   public:
    srAttribute(vkDevice device, uint attrib_id, size_t size,
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

struct srMesh : public Resource {
    srMesh(std::string name, uint32_t rid) : Resource(name, rid){};
    std::vector<srAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

struct srMeshHandle : public ResourceHandle {
    srMeshHandle() : ResourceHandle(){};
    srMeshHandle(size_t index, uint32_t rid) : ResourceHandle(index, rid){};
};

vkBuffer createIndexBuffer(vkDevice device,
                           const std::vector<std::list<uint>>& faces);

void destroyMesh(const vkDevice& device, const srMesh& mesh);

}  // namespace gbg
