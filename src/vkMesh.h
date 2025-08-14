#pragma once
#include <vulkan/vulkan_core.h>

#include <list>
#include <vector>

#include "vkBuffer.hh"
#include "vkDevice.hh"
namespace gbg {

struct vkAttribute {
   public:
    vkAttribute(vkDevice device, int attrib_id, size_t element_size,
                size_t element_count, void* data);

    virtual VkVertexInputBindingDescription getBindingDesc();
    virtual VkVertexInputAttributeDescription getAttribDesc();

   public:
    vkBuffer buffer;
    int attrib_id;
    size_t size;
};

struct vkMesh {
    std::vector<gbg::vkAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

vkBuffer createIndexBuffer(vkDevice device, std::vector<std::list<int>>& faces);

struct vkVector3Attribute : public vkAttribute {
   public:
    vkVector3Attribute(vkDevice device, int attrib_id, size_t element_size,
                       size_t element_count, void* data);
    virtual VkVertexInputBindingDescription getBindingDesc() override;
    virtual VkVertexInputAttributeDescription getAttribDesc() override;
};

}  // namespace gbg
