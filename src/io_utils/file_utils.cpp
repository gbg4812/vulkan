
#include "file_utils.hpp"

#include <filesystem>
#include <fstream>

namespace gbg {
std::vector<char> readFile(std::string_view filename) {
    std::ifstream file(filename.data());
    if (!file) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = std::filesystem::file_size(filename.data());
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

}  // namespace gbg
