

#pragma once
#include "vkDevice.hh"
#include "vkImage.h"

namespace gbg {

struct vkTexture {
    uint32_t mipLevels;
    gbg::vkImage textureImage;
    uint32_t sampler;
};

void generateMipmaps(vkDevice device, VkImage image, VkFormat format,
                     int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

}  // namespace gbg
