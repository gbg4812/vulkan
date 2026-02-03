#include "vkPipeline.hh"

#include <vulkan/vulkan_core.h>

#include <stdexcept>

#include "glm/glm.hpp"
#include "io_utils/file_utils.hpp"
#include "vk_utils/vkDevice.hh"

namespace gbg {
vkVertexInputDescription getVertexVector3InputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(glm::vec3);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
    desc.attrib_desc.offset = 0;
    return desc;
}
vkVertexInputDescription getVertexVector2InputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(glm::vec2);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32G32_SFLOAT;  // vec2 = 2 floats
    desc.attrib_desc.offset = 0;
    return desc;
}

vkVertexInputDescription getVertexFloatInputDescription(uint32_t attrib_id) {
    vkVertexInputDescription desc;
    desc.binding_desc.binding = attrib_id;
    desc.binding_desc.stride = sizeof(float);
    desc.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.attrib_desc.location = attrib_id;
    desc.attrib_desc.binding = attrib_id;
    desc.attrib_desc.format = VK_FORMAT_R32_SFLOAT;  // single float
    desc.attrib_desc.offset = 0;
    return desc;
}

VkShaderModule createShaderModule(const vkDevice& device,
                                  std::string_view code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device.ldevice, &createInfo, nullptr,
                             &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
    return shaderModule;
}

vkPipeline createGraphicsPipeline(
    const vkDevice& device, std::string_view vertShaderCode,
    std::string_view fragShaderCode,
    const std::vector<VkDescriptorSetLayout>& desc_sets_layouts,
    const std::vector<VkVertexInputBindingDescription>& binding_desc,
    const std::vector<VkVertexInputAttributeDescription>& attrib_desc,
    VkSampleCountFlagBits msaaSamples, VkRenderPass renderPass) {
    vkPipeline pipeline{};
    VkShaderModule vertShaderModule =
        createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule =
        createShaderModule(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = binding_desc.size();
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attrib_desc.size());
    vertexInputInfo.pVertexBindingDescriptions = binding_desc.data();
    vertexInputInfo.pVertexAttributeDescriptions = attrib_desc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = 0.2f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    // depthStencil.maxDepthBounds = 1.0f;
    // depthStencil.minDepthBounds = 0.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    // depthStencil.front = {};
    // depthStencil.back = {};

    // we perform alpha blending here
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // pseudo code explaining blendig operation
    // if (blendEnable) {
    //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb)
    //     <colorBlendOp> (dstColorBlendFactor * oldColor.rgb); finalColor.a
    //     = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp>
    //     (dstAlphaBlendFactor * oldColor.a);
    // } else {
    //     finalColor = newColor;
    // }
    //
    // finalColor = finalColor & colorWriteMask;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    const std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount =
        static_cast<uint32_t>(desc_sets_layouts.size());
    layoutCreateInfo.pSetLayouts = desc_sets_layouts.data();
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device.ldevice, &layoutCreateInfo, nullptr,
                               &pipeline.layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipeline.layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    // pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device.ldevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &pipeline.pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device.ldevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(device.ldevice, vertShaderModule, nullptr);

    return pipeline;
}
}  // namespace gbg
