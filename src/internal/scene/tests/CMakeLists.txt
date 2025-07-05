project(SceneTests CXX)


add_executable(meshtests MeshTests.cpp)
target_link_libraries(meshtests glm)
