# configure glm integration
add_subdirectory(glm)
list(APPEND LIBS glm::glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glm/glm/)
