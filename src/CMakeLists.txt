cmake_minimum_required(VERSION 3.14)

project(
    playfului
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

file(GLOB sources *.cpp)

add_executable(${PROJECT_NAME} ${sources})

if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -O0)
endif()


# External dependencies
find_package(Vulkan REQUIRED)

include_directories(./external)
include_directories(.)

# GLM
set(GLM_PTH ./external/glm)
add_subdirectory(${GLM_PTH})

include_directories(${GLM_PTH}/glm/)

set(GLFW_PTH ./external/glfw)

add_subdirectory(${GLFW_PTH})

include_directories(${GLFW_PTH}/include/)

set(LIBS "Vulkan::Vulkan;glfw;glm::glm")

target_link_libraries(${PROJECT_NAME} ${LIBS})

# link resources to the binary dir to be used
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND [ -h ${CMAKE_BINARY_DIR}/textures ] || ln -s ../../data/textures ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/shaders ] || ln -s ../../data/shaders ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/models ] || ln -s ../../data/models ${CMAKE_BINARY_DIR}/ || exit 0
)

# Test targets

add_executable(modeltests scene/tests/modeltests.cpp)

target_link_libraries(modeltests "glm::glm")
