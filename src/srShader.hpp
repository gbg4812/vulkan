#pragma once
#include <vulkan/vulkan_core.h>

#include "Resource.hpp"
#include "Shader.hpp"
#include "macros.hpp"
#include "vk_utils/vkPipeline.hh"

namespace gbg {
struct srShader : public Resource<ShaderHandle> {
    RESOURCE_CONSTR(srShader)
    vkPipeline pipeline;
    VkDescriptorSetLayout layout;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

inline const std::map<PrimitiveInterpretation, VkPrimitiveTopology>
    topologyToVulkan = {{TRIANGLES, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
                        {POINTS, VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
                        {LINES, VK_PRIMITIVE_TOPOLOGY_LINE_LIST}};

void destroySrShader(const vkDevice& device, const srShader& shader);

RELATED_RESOURCE_MANAGER(srShader, ShaderHandle);

}  // namespace gbg
