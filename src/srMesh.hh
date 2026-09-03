#pragma once
#include <vulkan/vulkan_core.h>

#include <list>
#include <vector>

#include "Mesh.hpp"
#include "Resource.hpp"
#include "macros.hpp"
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

struct srMesh : public Resource<MeshHandle> {
    RESOURCE_CONSTR(srMesh)
    std::vector<srAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

RELATED_RESOURCE_MANAGER(srMesh, MeshHandle);

std::vector<uint32_t> createIndexBuffer(
    vkDevice device, const std::vector<std::list<uint>>& faces);

std::vector<glm::vec3> createTangentBuffer(
    vkDevice device, const std::vector<glm::vec3>& pos,
    const std::vector<glm::vec2> tex_coord,
    const std::vector<uint32_t> indices);
void destroyMesh(const vkDevice& device, const srMesh& mesh);

}  // namespace gbg
