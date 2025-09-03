add_library(gbgscene INTERFACE)
target_include_directories(gbgscene INTERFACE .)

add_subdirectory(./external)
target_link_libraries(gbgscene INTERFACE glm::glm stbimage tinyobjloader)


# Test targets
add_executable(modeltests tests/modeltests.cpp)
target_link_libraries(modeltests gbgscene)
