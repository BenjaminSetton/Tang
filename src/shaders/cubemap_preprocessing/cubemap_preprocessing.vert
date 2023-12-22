#version 450

//
// These shaders convert equirectangular HDR textures to cubemaps by
// projecting the equirectangular texture to a unit cube. Since we render
// to each face, we can take all 6 faces to complete our cubemap.
// This process is better explained in LearnOpenGL:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
//

layout (location = 0) in vec3 modelPos;

layout (location = 0) out vec3 localPos;

layout(set = 0, binding = 0) uniform ProjObject {
    mat4 proj;
} projUBO;

layout(set = 0, binding = 1) uniform ViewObject {
    mat4 view;
} viewUBO;

void main()
{
    localPos = modelPos;
    gl_Position =  projUBO.proj * viewUBO.view * vec4(localPos, 1.0);
}