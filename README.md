# Vulkan Renderer

## About the project

This project is _me learning vulkan_. It started as me following the Vulkan
Tutorial and has evolved into a personal project with the objective of having _a
personal graphics toolkit to play and learn_. For now is just a _work in
progress_ of a library so my main test is the file app.cpp in the tests/
directory.

## Technologies

- The toolkit uses Vulkan API as graphics API and glfw for the context and input.
  It uses my other library
- [SceneEquipament](https://github.com/gbg4812/SceneEquipament.git) scene input to
  render.
- [Tracy](https://github.com/wolfpld/tracy) to profile performance
- [Renderdoc](https://renderdoc.org/) (not instrumented) for graphics debugging

## Screenshots

![image](LittleLightingSS.png)

## Features

### Rendering of 3D scenes descrived with SceneEquipament library

- Model matrices from the scene graph
- Rendering of meshes (with any combination of attributes)
- SPIR-V shaders with realtime updating
- Shader inputs defined by materials including numeric types and Textures (with realtime updating too)
- Perspective Camera (relatime updating)
- Shadow Mapping (if the shader cooperates)
- Multiple point lights (if the shader cooperates)

And overall flexibility to extend and implement more visual effects.

## To build

### Prerequisits

- The vulkan sdk
- Network Connection

## Build Instructions

To build the library and the test app.

```bash
cmake -B build
cmake --build build --parallel
```

## Integration Instructions

Just add the repository directory in cmake and add the `VkSceneRenderEngine` target
as a library to link.

## Project Structure:

- **src/** where all the source code is.
- **src/external** repositoris of external dependencies like stb_image.
- **src/vk_utils** vulkan utility functions and structures (my be some day I will
  separate them into their own library).
- **test/** mainly the _app_ executable wich is a demo app to showcase and test the renderer.
- **Notes.md** some notes on the design and vulkan things I have learned along the path way.
