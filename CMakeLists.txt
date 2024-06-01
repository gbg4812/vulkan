cmake_minimum_required(VERSION 3.14)

project(
    playfului
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(${PROJECT_NAME}_SRC 
    src/VulkanEngine.cpp
    src/VulkanExtras.cpp
    src/Vertex.cpp
    src/Object.cpp
    src/vkutils.cpp
)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})

if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -O0)
endif()

find_package(VulkanHeaders REQUIRED)
find_package(glm REQUIRED)
find_package(stb REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(${PROJECT_NAME} vulkan-headers::vulkan-headers)
target_link_libraries(${PROJECT_NAME} glm::glm)
target_link_libraries(${PROJECT_NAME} stb::stb)
target_link_libraries(${PROJECT_NAME} tinyobjloader::tinyobjloader)
target_link_libraries(${PROJECT_NAME} glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)



# link resources to the binary dir to be used
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND [ -h ${CMAKE_BINARY_DIR}/textures ] || ln -s ${PROJECT_SOURCE_DIR}/data/textures ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/shaders ] || ln -s ${PROJECT_SOURCE_DIR}/data/shaders ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/models ] || ln -s ${PROJECT_SOURCE_DIR}/data/models ${CMAKE_BINARY_DIR}/ || exit 0
)
