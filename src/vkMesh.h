#include <vulkan/vulkan_core.h>

#include <vector>

#include "vkBuffer.h"
namespace gbg {

struct vkAttribute {
   public:
    vkAttribute(int attrib_id, size_t element_size, size_t element_count,
                void* data);

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

}  // namespace gbg
