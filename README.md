# Vulkan Wraper + Scene Representation

## About the project

This project is me learning vulkan. It started as me following the Vulkan
Tutorial and has evolved into a personal project with the objective of having _a
personal graphics toolkit to play and learn_. For now is just a _work in
progress_ of a library so my main test is the file app.cpp in the src/
directory.

## Technologies

The toolkit uses Vulkan API as graphics API and glfw for the context and input.

## Organitzation

Directories:

- src/ where all the source code is.
- src/external repositoris of external dependencies like glfw or glm.
- src/internal other sub-libraries made by me like the scene representation.
- src/internal/scene the scene representation library with a loader for meshes
