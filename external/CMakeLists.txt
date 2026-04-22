add_subdirectory(./stb_image)


file(GLOB IMGUI_SOURCES imgui/*.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_vulkan.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC imgui)
target_link_libraries(imgui PUBLIC glfw)
