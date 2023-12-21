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

layout(set = 0, binding = 0) uniform ViewProjUBO 
{
    mat4 view;
    mat4 proj;
} viewProjUBO;

void main()
{
    localPos = modelPos;
    gl_Position =  viewProjUBO.proj * viewProjUBO.view * vec4(localPos, 1.0);
}