#include "srShader.hpp"

namespace gbg {

void destroySrShader(const vkDevice& device, const srShader& shader) {
    vkDestroyPipeline(device.ldevice, shader.pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device.ldevice, shader.pipeline.layout, nullptr);
    vkDestroyDescriptorSetLayout(device.ldevice, shader.layout, nullptr);
}

}  // namespace gbg
