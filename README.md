# TANG
A simple graphics framework that allows any project to quickly display stuff on the screen

![Tang Logo](src/data/logo.png)

## DISCLAIMER

**This project currently supports Windows only!**

## INSTALLATION

1. Go to the "Releases" tab (<INSERT_LINK>)
2. Install the latest release version

NOTES:
- The release download will come with two things: the .lib file and a folder containing the headers. Alternatively, you can clone the repository and compile the source code yourself (look at "Compiling the sources")

## COMPILING THE SOURCES

1. TODO

## RESOURCES

- Vulkan Tutorial - https://vulkan-tutorial.com
- GLFW input - https://www.glfw.org/docs/3.3/input_guide.html#input_keyboard
- Amazing Vulkan tutorials by Brendan Galea - https://www.youtube.com/@BrendanGalea
- Vulkan abstraction guide - https://vkguide.dev
- Image-based lighting - https://www.youtube.com/watch?v=xWCZiksqCGA
- PBR explanation + walk-through (Victor Gordan) - https://www.youtube.com/watch?v=RRE-F57fbXw&t=10s
- PBR explanation + walk-through (OGL Dev) - https://www.youtube.com/watch?v=XK_p2MxGBQs&t=126s
- Normal mapping (LearnOpenGL) - https://learnopengl.com/Advanced-Lighting/Normal-Mapping

## DEPENDENCIES

- Vulkan SDK (graphics API)
- GLFW (OS window handling)
- GLM (math library)
- stb (image loading)
- assimp (asset importing)

## TODO

- [X] Abstract away all the GLFW window calls into their own Window or Surface class
- [X] Come up with a better abstraction for descriptor sets, pools, set layouts and uniform buffers. At the moment it's really tedious to add/remove or update data we're sending to the shaders because it must be changed it too many places
- [X] Write a basic Camera class that allows us to move around the scene
- [X] Gather keyboard input using GLFW
- [X] Gather mouse input using GLFW
- [X] Create an abstraction for textures. These must be imported, loaded, and uploaded to the shaders
- [X] Implement a PBR shader
- [X] Re-organize the renderer code to allow a PBR pipeline
- [X] Move project build scripts from CMake to Premake
- [X] Create a default material so that assets with incomplete/missing PBR textures can be loaded
- [ ] Make shader information more data-driven (e.g. pull shader's input uniform + texture information from a set of data)
- [ ] Re-organize the renderer's frame-dependent data (FDD) and swap-chain-dependent data (SWIDD)
- [ ] Add HDR support
- [ ] Add cubemap support in preparation for IBL
- [ ] Implement diffuse IBL
- [ ] Implement specular IBL
- [ ] Clarify boundary between asset loader and renderer (especially in naming convention)