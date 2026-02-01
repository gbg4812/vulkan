include(FetchContent)
cmake_minimum_required(VERSION 3.14)

project(
    VkSceneRenderEngine
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

file(GLOB sources src/*.cpp src/vk_utils/*.cpp)

add_library(${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PUBLIC src/)

# External dependencies
find_package(Vulkan REQUIRED)

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

fetchcontent_declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
fetchcontent_makeavailable(fmt)

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

set(LIBS "Vulkan::Vulkan;glfw;glm::glm;stbimage;fmt::fmt;SceneEquipament")

target_link_libraries(${PROJECT_NAME} ${LIBS})

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

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/tests/data ${CMAKE_BINARY_DIR}/data SYMBOLIC)

add_executable(app tests/app.cpp)

target_link_libraries(app PRIVATE ${PROJECT_NAME})



