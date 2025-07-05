#include <vulkan/vulkan_core.h>

#include <vector>

#include "vkBuffer.h"
namespace gbg {

struct vkAttribute {
    VkVertexInputBindingDescription bindingDesc;
    VkVertexInputAttributeDescription attribDesc;

   protected:
    vkAttribute();
};

class vkVector2Attribute : vkAttribute {
   public:
    vkVector2Attribute(int location);
    vkBuffer buffer;
};

struct vkMesh {
    std::vector<gbg::vkAttribute> vertexAttributes;
    gbg::vkBuffer indexBuffer;
};

}  // namespace gbg
