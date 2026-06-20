
#include "file_utils.hpp"

#include <fstream>

namespace gbg {
std::vector<char> readFile(std::string_view filename) {
    std::ifstream file(filename.data(), std::ios::in | std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize + 1);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), fileSize + 1);
    file.close();
    return buffer;
}

}  // namespace gbg
