

#ifndef GBG_VKTEXTURE
#define GBG_VKTEXTURE

#include "vkImage.h"

namespace gbg {

struct vkTexture {
    uint32_t mipLevels;
    gbg::vkImage textureImage;
    uint32_t sampler;
};

}  // namespace gbg

#endif
