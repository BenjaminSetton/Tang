# TANG
A simple graphics framework that allows any project to quickly display stuff on the screen

![Tang Logo](logo.png)

## DISCLAIMER

**This project currently supports Windows only!**

## INSTALLATION

1. Go to the "Releases" tab (<INSERT_LINK>)
2. Install the latest release version

NOTES:
- The release download will come with two things: the .lib file and a folder containing the headers. Alternatively, you can clone the repository and compile the source code yourself (look at "Compiling the sources")
- This project also includes a demo scene that demonstrates some of the capabilities of the renderer. Looking through the demo scene's code should also paint a pretty good picture of how to use the renderer

## COMPILING THE SOURCES

1. Clone this repository "```git clone https://github.com/BenjaminSetton/TANG.git```" (no submodules)
2. Edit `TANG/tools/build_project.bat`
3. Change the generator to your favorite IDE
4. Run `TANG/tools/build_project.bat`

## RESOURCES

- Vulkan Tutorial - https://vulkan-tutorial.com
- GLFW input - https://www.glfw.org/docs/3.3/input_guide.html#input_keyboard
- Amazing Vulkan tutorials by Brendan Galea - https://www.youtube.com/@BrendanGalea
- Vulkan abstraction guide - https://vkguide.dev
- Image-based lighting - https://www.youtube.com/watch?v=xWCZiksqCGA
- PBR explanation + walk-through (Victor Gordan) - https://www.youtube.com/watch?v=RRE-F57fbXw&t=10s
- PBR explanation + walk-through (OGL Dev) - https://www.youtube.com/watch?v=XK_p2MxGBQs&t=126s
- Shader resource bindings - https://developer.nvidia.com/vulkan-shader-resource-binding
- Sync objects - https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
- Raytracing tutorial - https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
- Fixing fireflies when convoluting HDR maps - https://graphicrants.blogspot.com/2013/12/tone-mapping.html?m=1
- Free PBR materials - https://freepbr.com
- Free 3D assets - https://polyhaven.com
- LearnOpenGL (tutorials) - https://learnopengl.com
- LearnOpenGL (code repo) - https://github.com/JoeyDeVries/LearnOpenGL
- Writing an efficient Vulkan renderer - https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

## DEPENDENCIES

- Vulkan SDK (graphics API)
- GLFW (OS window handling)
- GLM (math library)
- stb_image (image loading)
- assimp (asset importing)
- nlohmann_json (JSON parsing)

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
- [X] Add HDR support
- [X] Add cubemap support in preparation for IBL
- [X] Implement diffuse IBL
- [X] Implement specular IBL
- [ ] Make shader information more data-driven (e.g. pull shader's input uniform + texture information from a set of data)
- [ ] Re-organize the renderer's frame-dependent data (FDD) and swap-chain-dependent data (SWIDD)
- [ ] Clarify boundary between asset loader and renderer (especially in naming convention)