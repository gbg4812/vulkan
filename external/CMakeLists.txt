# configure glm integration
add_subdirectory(glm)
list(APPEND LIBS glm::glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glm/)

# configure tinybojetmodel integration
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/)

# configure stb_image integration 

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stb_image/)
