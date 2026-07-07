#include "resourcesUpdate.hpp"

#include <string.h>
#include <vulkan/vulkan_core.h>

#include <ranges>

#include "PerObjectPushConstant.hpp"
#include "Resource.hpp"
#include "vk_utils/vkCommandBuffer.hh"
#include "vk_utils/vkImage.hh"

namespace gbg {

void updateShader(vkDevice device, ShaderHandle sh_h,
                  InternalSceneData& scene_data, vkRenderPass renderPass, std::vector<VkDescriptorSetLayout> rendererDescriptorSetLayouts) {
    Shader& shader = scene_data.scene->sh_mg.get(sh_h);
    uint32_t flags = shader.getFlags();
    if (flags & ResourceFlags::NEW)
        srShaderHandle shh =
            scene_data.srsh_mg.create("srShader::" + shader.getName());
    srShader& sr_sh = scene_data.srsh_mg.getRelated(sh_h);

    if (flags & (ResourceFlags::NEW | ResourceFlags::DIRTY)) {
        if (flags & ResourceFlags::DIRTY) {
            vkDeviceWaitIdle(device.ldevice);
            vkDestroyDescriptorSetLayout(device.ldevice, sr_sh.layout, nullptr);
            vkDestroyPipeline(device.ldevice, sr_sh.pipeline.pipeline, nullptr);
            vkDestroyPipelineLayout(device.ldevice, sr_sh.pipeline.layout,
                                    nullptr);
        }

        sr_sh.topology = topologyToVulkan.at(shader.topology);

        std::vector<VkDescriptorSetLayoutBinding> materialBindings;
        if (not shader.getParameters().empty()) {
            VkDescriptorSetLayoutBinding matParmsLayoutBinding{};
            matParmsLayoutBinding.binding = 0;
            matParmsLayoutBinding.descriptorCount = 1;
            matParmsLayoutBinding.descriptorType =
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            matParmsLayoutBinding.pImmutableSamplers = nullptr;
            matParmsLayoutBinding.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            materialBindings.push_back(matParmsLayoutBinding);
        }

        auto texFilter = [](ParameterTypes p) {
            return p == ParameterTypes::TEXTURE_PARM;
        };

        // creates a binding for each texture
        int textureCount = 0;
        for (ParameterTypes p :
             shader.getParameters() | std::ranges::views::filter(texFilter)) {
            textureCount++;
        }

        if (textureCount) {
            VkDescriptorSetLayoutBinding texBinding{};
            texBinding.binding = 1;
            texBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            texBinding.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            texBinding.descriptorCount = textureCount;
            texBinding.pImmutableSamplers = nullptr;
            materialBindings.push_back(texBinding);
        }


        if (not materialBindings.empty()) {
            VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
            materialLayoutInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            materialLayoutInfo.bindingCount =
                static_cast<uint32_t>(materialBindings.size());
            materialLayoutInfo.pBindings = materialBindings.data();

            if (vkCreateDescriptorSetLayout(device.ldevice, &materialLayoutInfo,
                                            nullptr,
                                            &sr_sh.layout) != VK_SUCCESS) {
                throw std::runtime_error(
                    "failed to create descriptor set layout!");
            }
            rendererDescriptorSetLayouts.insert(++rendererDescriptorSetLayouts.begin(), sr_sh.layout);
        }

        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        // TODO: make them a parameter.
        for (const auto& type : shader.getAttributes()) {
            vkVertexInputDescription desc;
            switch (type.second) {
                case FLOAT_ATTR:
                    desc = getVertexFloatInputDescription(type.first);
                    break;
                case VEC2_ATTR:
                    desc = getVertexVector2InputDescription(type.first);
                    break;
                case VEC3_ATTR:
                    desc = getVertexVector3InputDescription(type.first);
                    break;
            }
            bindingDescriptions.push_back(desc.binding_desc);
            attributeDescriptions.push_back(desc.attrib_desc);
        }

        // for the model matrix
        VkPushConstantRange mdl_rg{};
        mdl_rg.offset = 0;
        mdl_rg.size = sizeof(PerObjectPushConstant);
        mdl_rg.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::vector<VkPushConstantRange> push_constants = {mdl_rg};

        sr_sh.pipeline = createGraphicsPipeline(
            device, shader.getVertShaderCode(), shader.getFragShaderCode(),
            rendererDescriptorSetLayouts, bindingDescriptions, attributeDescriptions,
            push_constants, renderPass.samples, renderPass.renderPass,
            sr_sh.topology);
    }
}

void updateMesh(vkDevice device, MeshHandle mesh_h,
                InternalSceneData& scene_data) {
    Scene* scene = scene_data.scene;
    auto& mesh = scene->ms_mg.get(mesh_h);

    auto flags = mesh.getFlags();
    if (flags & ResourceFlags::NEW) {
        srMeshHandle vkmh =
            scene_data.srmsh_mg.create("srMesh::" + mesh.getName());
        srMesh& vkmesh = scene_data.srmsh_mg.getRelated(mesh_h);

        for (auto& attr : mesh.getAttributes()) {
            srAttribute attrib = std::visit<srAttribute>(
                [&](auto&& arg) -> srAttribute {
                    return srAttribute(device, attr.first, arg.size(),
                                       (AttributeTypes)attr.second.index(),
                                       (void*)arg.data());
                },
                attr.second);

            vkmesh.vertexAttributes.push_back(attrib);
        }

        std::vector<uint32_t> indices =
            createIndexBuffer(device, mesh.getFaces());

        VkDeviceSize size = indices.size() * sizeof(indices[0]);

        gbg::vkBuffer stagingBuffer =
            gbg::createBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device.ldevice, stagingBuffer.memory, 0, size, 0, &data);
        memcpy(data, indices.data(), size);
        vkUnmapMemory(device.ldevice, stagingBuffer.memory);

        vkBuffer indexBuffer = gbg::createBuffer(
            device, size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        copyBuffer(device, stagingBuffer, indexBuffer);
        vkDestroyBuffer(device.ldevice, stagingBuffer.buffer, nullptr);
        vkFreeMemory(device.ldevice, stagingBuffer.memory, nullptr);

        vkmesh.indexBuffer = indexBuffer;

        auto tangents = createTangentBuffer(
            device, mesh.getAttribute<AttributeTypes::VEC3_ATTR>(0),
            mesh.getAttribute<AttributeTypes::VEC2_ATTR>(2), indices);

        auto tangentAttr =
            srAttribute(device, mesh.getAttributes().size(), tangents.size(),
                        AttributeTypes::VEC3_ATTR, (void*)tangents.data());
        vkmesh.vertexAttributes.push_back(tangentAttr);
    }
}

void updateMaterial(vkDevice device, MaterialHandle math,
                    InternalSceneData& scene_data,
                    VkDescriptorPool materialDescPool,
                    VkSampler textureSampler) {
    Material& mat = scene_data.scene->mat_mg.get(math);
    if (mat.getFlags() & ResourceFlags::NEW)
        srMaterialHandle mth =
            scene_data.srmat_mg.create("srMaterial::" + mat.getName());

    if (mat.getFlags() & (ResourceFlags::NEW | ResourceFlags::DIRTY)) {
        // TODO: easy to leak memory
        srMaterial& srmt = scene_data.srmat_mg.getRelated(math);

        // we have the data layed out
        srParameterValues values = gbg::allocateParameterValues(mat);

        if (values.size > 0) {
            if (mat.getFlags() & ResourceFlags::NEW) {
                srmt.paramBuffer = gbg::createBuffer(
                    device, values.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
            void* data;
            vkMapMemory(device.ldevice, srmt.paramBuffer.memory, 0,
                        srmt.paramBuffer.size, 0, &data);
            memcpy(data, values.data, values.size);
            vkUnmapMemory(device.ldevice, srmt.paramBuffer.memory);

            delete values.data;
        }

        auto& sh = scene_data.scene->sh_mg.get(mat.getShaderHandle());
        // create descriptor sets if new
        if (mat.getFlags() & ResourceFlags::NEW)
            createMaterialDescriptorSet(device, math, scene_data,
                                        materialDescPool);
        else if (sh.getFlags() & ResourceFlags::DIRTY) {
            vkDeviceWaitIdle(device.ldevice);
            vkFreeDescriptorSets(device.ldevice, materialDescPool, 1,
                                 &srmt.descriptor_set);
            createMaterialDescriptorSet(device, math, scene_data,
                                        materialDescPool);
        }

        updateMaterialDescriptorSet(device, math, scene_data, textureSampler);
    }
}

void updateMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                 InternalSceneData& scene_data,
                                 VkSampler textureSampler) {
    // no volem cap frame dibuixant-se
    vkDeviceWaitIdle(device.ldevice);

    auto& srmat = scene_data.srmat_mg.getRelated(h);
    auto& mat = scene_data.scene->mat_mg.get(h);

    std::vector<VkWriteDescriptorSet> descWrites = {};

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = srmat.paramBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    if (not mat.getValues().empty()) {
        VkWriteDescriptorSet writeDesc{};

        writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDesc.dstSet = srmat.descriptor_set;
        writeDesc.dstBinding = 0;
        writeDesc.dstArrayElement = 0;
        writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDesc.descriptorCount = 1;
        writeDesc.pImageInfo = nullptr;
        writeDesc.pTexelBufferView = nullptr;
        writeDesc.pBufferInfo = &bufferInfo;
        descWrites.push_back(writeDesc);
    }

    std::vector<VkDescriptorImageInfo> imageInfos;

    // fun range stuff!
    for (const parm_vt& val : mat.getValues()) {
        if (auto th = std::get_if<TextureHandle>(&val)) {
            VkDescriptorImageInfo imageInfo{};
            if (*th) {
                imageInfo.imageView = scene_data.srtx_mg.getRelated(*th)
                                          .textureImage.view.value();
            } else {
                imageInfo.imageView = scene_data.srtx_mg.getRelated(TextureHandle(1,1))
                                          .textureImage.view.value();
            }
            imageInfo.sampler = textureSampler;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos.push_back(imageInfo);
        }
    }

    if (imageInfos.size() > 0) {
        VkWriteDescriptorSet writeDesc{};
        writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDesc.dstSet = srmat.descriptor_set;
        writeDesc.dstBinding = 1;
        writeDesc.dstArrayElement = 0;
        writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDesc.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writeDesc.pImageInfo = imageInfos.data();
        writeDesc.pTexelBufferView = nullptr;
        writeDesc.pBufferInfo = nullptr;
        descWrites.push_back(writeDesc);
    }

    if (descWrites.empty()) return;
    vkUpdateDescriptorSets(device.ldevice, descWrites.size(), descWrites.data(),
                           0, nullptr);
}

void createMaterialDescriptorSet(vkDevice device, MaterialHandle h,
                                 InternalSceneData& scene_data,
                                 VkDescriptorPool materialDescPool) {
    auto& mat = scene_data.scene->mat_mg.get(h);
    if (mat.getValues().empty()) return;  // has no parameters or textures
    ShaderHandle shh = mat.getShaderHandle();

    srShader& srsh = scene_data.srsh_mg.getRelated(shh);
    srMaterial& srmat = scene_data.srmat_mg.getRelated(h);

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = materialDescPool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &srsh.layout;

    if (vkAllocateDescriptorSets(device.ldevice, &setInfo,
                                 &srmat.descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets");
    }
}

void updateTexture(vkDevice device, TextureHandle h,
                   InternalSceneData& scene_data, VkSampler textureSampler) {
    auto& texture = scene_data.scene->tx_mg.get(h);
    auto flags = texture.getFlags();

    if (flags & NEW) {
        if (h) {
            srTextureHandle tex_h = scene_data.srtx_mg.create("srTexture" + texture.getName());
        } else {
            h = scene_data.scene->defaults.texture;
        }
        auto& tex = scene_data.srtx_mg.getRelated(h);

        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        if (texture.raw) format = VK_FORMAT_R8G8B8A8_UNORM;

        tex.textureImage = createImage(
            device.pdevice, device.ldevice,
            static_cast<uint32_t>(texture.width),
            static_cast<uint32_t>(texture.height),
            static_cast<uint32_t>(texture.mip_levels), VK_SAMPLE_COUNT_1_BIT,
            format, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        tex.mipLevels = texture.mip_levels;
        tex.sampler = textureSampler;

        addImageView(tex.textureImage, device.ldevice, format,
                     VK_IMAGE_ASPECT_COLOR_BIT, tex.mipLevels);

        VkDeviceSize dsize = texture.data.size();

        gbg::vkBuffer stagingBuffer =
            gbg::createBuffer(device, dsize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void* sdata;
        vkMapMemory(device.ldevice, stagingBuffer.memory, 0, dsize, 0, &sdata);
        memcpy(sdata, texture.data.data(), texture.data.size());
        vkUnmapMemory(device.ldevice, stagingBuffer.memory);
        
        VkCommandBuffer transBuffer = beginSingleTimeCommands(device, device.transferCmdPool);

        transitionImageLayout(device, transBuffer, tex.textureImage.image, format,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              static_cast<uint32_t>(tex.mipLevels));
        
        endSingleTimeCommands(device, transBuffer, device.transferCmdPool, device.tqueue);

        copyBufferToImage(device, stagingBuffer.buffer, tex.textureImage.image,
                          texture.width, texture.height);
        vkDestroyBuffer(device.ldevice, stagingBuffer.buffer, nullptr);
        vkFreeMemory(device.ldevice, stagingBuffer.memory, nullptr);

        transBuffer = beginSingleTimeCommands(device, device.transferCmdPool);

        transitionImageLayout(device, transBuffer, tex.textureImage.image, format,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              static_cast<uint32_t>(tex.mipLevels));

        endSingleTimeCommands(device, transBuffer, device.transferCmdPool, device.tqueue);
    }
}

}  // namespace gbg
