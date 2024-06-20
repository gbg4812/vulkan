cmake_minimum_required(VERSION 3.14)

project(
    playfului
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(SRC 
    src/VulkanEngine.cpp
    src/VulkanExtras.cpp
    src/Vertex.cpp
    src/Object.cpp
    src/vkutils.cpp
)

add_library(${PROJECT_NAME} ${SRC})


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/lib)

set(LIBS glfw3 vulkan Xi Xrandr Xcursor Xinerama)

# update the LIBS list with external libs
add_subdirectory(external)

target_link_libraries(${PROJECT_NAME} ${LIBS})

# link resources to the binary dir to be used
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND [ -h ${CMAKE_BINARY_DIR}/textures ] || ln -s ${CMAKE_SOURCE_DIR}/data/textures ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/shaders ] || ln -s ${CMAKE_SOURCE_DIR}/data/shaders ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/models ] || ln -s ${CMAKE_SOURCE_DIR}/data/models ${CMAKE_BINARY_DIR}/ || exit 0
)
