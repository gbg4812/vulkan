cmake_minimum_required(VERSION 3.0.0)

project(LearnVulkan)
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(srcs app.cc)


add_executable(app "${srcs}")

set(CMAKE_CTEST_ARGUMENTS "--verbose")
add_test(NAME CreateWindow COMMAND app)

include_directories(include)

target_link_directories(app PRIVATE lib)
target_link_libraries(app glfw3 vulkan dl pthread X11 Xxf86vm Xrandr Xi)


