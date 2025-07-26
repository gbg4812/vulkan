#include "vkMesh.h"

namespace gbg {
vkAttribute::vkAttribute(int attrib_id, size_t element_size,
                         size_t element_count, void* data)
    : attrib_id(attrib_id), size(element_size * element_count) {}

}  // namespace gbg
