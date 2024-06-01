from conan import ConanFile
from conan.tools.cmake import cmake_layout

class PlayfulUi(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("vulkan-headers/1.3.239.0")
        self.requires("vulkan-loader/1.3.239.0")
        self.requires("vulkan-validationlayers/1.3.239.0")
        self.requires("glfw/3.4")
        self.requires("glm/cci.20230113")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("stb/cci.20240213")

    def layout(self):
        cmake_layout(self)
