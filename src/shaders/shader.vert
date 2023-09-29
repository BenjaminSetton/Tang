#version 450

layout(set = 0, binding = 0) uniform ViewProjObject {
    mat4 view;
    mat4 proj;
} viewProjUBO;

layout(set = 1, binding = 0) uniform TransformObject {
    mat4 transform;
} transformUBO;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

void main() {
    gl_Position = viewProjUBO.proj * viewProjUBO.view * transformUBO.transform * vec4(inPosition, 1.0);

    outNormal = (transformUBO.transform * vec4(inNormal, 1.0)).xyz;
    outUV = inUV;
}