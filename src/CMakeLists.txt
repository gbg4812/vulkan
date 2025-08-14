include(FetchContent)
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

fetchcontent_declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
fetchcontent_makeavailable(fmt)

target_include_directories(${PROJECT_NAME} PUBLIC .)

add_subdirectory(./external)
add_subdirectory(./internal)

set(LIBS "Vulkan::Vulkan;glfw;glm::glm;stbimage;tinyobjloader;gbgscene;fmt::fmt")

target_link_libraries(${PROJECT_NAME} ${LIBS})

# link resources to the binary dir to be used
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND [ -h ${CMAKE_BINARY_DIR}/textures ] || ln -s ../../data/textures ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/shaders ] || ln -s ../../data/shaders ${CMAKE_BINARY_DIR}/ || exit 0
    COMMAND [ -h ${CMAKE_BINARY_DIR}/models ] || ln -s ../../data/models ${CMAKE_BINARY_DIR}/ || exit 0
)

