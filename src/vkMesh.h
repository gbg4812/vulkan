#include <vulkan/vulkan_core.h>

#include <vector>

#include "vkBuffer.h"
namespace gbg {

struct vkAttribute {
   public:
    vkAttribute(int location, int binding, size_t element_size,
                size_t element_count, void* data);

    virtual VkVertexInputBindingDescription getBindingDesc();
    virtual VkVertexInputAttributeDescription getAttribDesc();

   public:
    vkBuffer buffer;
};

struct vkMesh {
    std::vector<gbg::vkAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

}  // namespace gbg
