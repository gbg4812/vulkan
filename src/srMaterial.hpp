
#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "vk_utils/vkBuffer.hh"

namespace gbg {

struct srParameterValues {
    void* data;
    size_t size;
};

struct srMaterial {
    VkDescriptorSet descriptor_set;
    vkBuffer paramBuffer;
    srParameterValues parameter_values;
};

srParameterValues allocateParameterValues(Material& model);

}  // namespace gbg
