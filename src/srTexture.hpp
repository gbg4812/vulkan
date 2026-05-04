

#pragma once
#include "Resource.hpp"
#include "macros.hpp"
#include "vk_utils/vkDevice.hh"
#include "vk_utils/vkImage.hh"

namespace gbg {

struct srTexture : public Resource {
    RESOURCE_CONSTR(srTexture)

    uint32_t mipLevels;
    gbg::vkImage textureImage;
    VkSampler sampler;
};

void generateMipmaps(vkDevice device, VkImage image, VkFormat format,
                     int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

RESOURCE_HANDLE(srTextureHandle);

}  // namespace gbg
