

#pragma once
#include "Resource.hpp"
#include "Texture.hpp"
#include "macros.hpp"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.hh"

namespace gbg {

struct srTexture : public Resource<TextureHandle> {
    RESOURCE_CONSTR(srTexture)

    uint32_t mipLevels;
    gbg::vkImage textureImage;
    VkSampler sampler;
};

void generateMipmaps(vkDevice device, VkImage image, VkFormat format,
                     int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

void destroySrTexture(const vkDevice& device, const srTexture& texture);

RELATED_RESOURCE_MANAGER(srTexture, TextureHandle);

}  // namespace gbg
