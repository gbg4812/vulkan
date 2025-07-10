
#ifndef GBG_RESLOD
#define GBG_RESLOD
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <string>

#include "scene/Scene.hpp"

namespace gbg {

void loadModel(Scene& scene, std::string model_path);

Image loadTexture(Scene& scene, std::string texture_path);

}  // namespace gbg

#endif
