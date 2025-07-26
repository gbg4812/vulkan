
#ifndef GBG_RESLOD
#define GBG_RESLOD
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <memory>
#include <string>

#include "Model.hpp"

namespace gbg {

std::shared_ptr<Mesh> loadMesh(std::string model_path);

std::shared_ptr<Image> loadTexture(std::string texture_path);

}  // namespace gbg

#endif
