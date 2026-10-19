include(FetchContent)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
    VkSceneRenderEngine
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

file(GLOB sources src/*.cpp src/vk_utils/*.cpp src/io_utils/*.cpp src/geoc/*.cpp)

add_library(${PROJECT_NAME} ${sources})

set(CONTENTS_DIR ${CMAKE_SOURCE_DIR}/contents/)
configure_file(src/config/build_constants.hpp.in config/build_constants.hpp)

target_include_directories(${PROJECT_NAME} PUBLIC src/ ${CMAKE_CURRENT_BINARY_DIR}/config)

# External dependencies

FetchContent_Declare (
tracy
GIT_REPOSITORY https://github.com/wolfpld/tracy.git
GIT_TAG v0.13.1
GIT_SHALLOW TRUE
GIT_PROGRESS TRUE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_tracy_default ON)
else()
    set(_tracy_default OFF)
endif()

option(TRACY_ENABLE " " ${_tracy_default})
option(TRACY_ON_DEMAND " " ON)
FetchContent_MakeAvailable(tracy)

find_package(Vulkan REQUIRED)
find_package(slang CONFIG HINTS "$ENV{VULKAN_SDK}/lib/cmake" REQUIRED)

if(LOCAL_SEQ)
  fetchcontent_declare(
  sceneeq
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../SceneEquipament/)
endif()

fetchcontent_declare(
  sceneeq
  GIT_REPOSITORY https://github.com/gbg4812/SceneEquipament.git
  GIT_TAG        master)
fetchcontent_makeavailable(sceneeq)

FetchContent_Declare(
    glm
    GIT_REPOSITORY	https://github.com/g-truc/glm.git
    GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG master)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_subdirectory(./external)

# libs
list(APPEND LIBS Vulkan::Vulkan slang::slang glfw)
list(APPEND LIBS glm::glm stbimage)
list(APPEND LIBS imgui SceneEquipament)

# dev only
list(APPEND DEV_LIBS TracyClient)

target_link_libraries(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:${DEV_LIBS}> ${LIBS})


###########
# Testing #
###########

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# The test app target
FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
  GIT_TAG v1.3.0
)

FetchContent_MakeAvailable(nfd)

add_executable(app tests/app.cpp)

target_link_libraries(app PRIVATE "${PROJECT_NAME};nfd")

include(CMakePrintHelpers)

# Print the C++ compiler path, ID, and version
cmake_print_variables(CMAKE_CXX_COMPILER)
cmake_print_variables(CMAKE_CXX_COMPILER_ID)
cmake_print_variables(CMAKE_CXX_COMPILER_VERSION)

# Print global C++ compilation flags
cmake_print_variables(CMAKE_CXX_FLAGS)
cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
cmake_print_variables(CMAKE_CXX_FLAGS_RELEASE)
