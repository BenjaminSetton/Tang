#version 450

layout(set = 1, binding = 0) uniform ProjObject {
    mat4 proj;
} projUBO;

layout(set = 2, binding = 2) uniform ViewObject {
    mat4 view;
} viewUBO;

layout(set = 2, binding = 0) uniform TransformObject {
    mat4 transform;
} transformUBO;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main() {
    gl_Position = projUBO.proj * viewUBO.view * transformUBO.transform * vec4(inPosition, 1.0);

    // Calculate the output variables going to the pixel shader
    outPosition = (viewUBO.view * transformUBO.transform * vec4(inPosition, 1.0)).xyz;

    outNormal = (transformUBO.transform * vec4(inNormal, 0.0)).xyz;
    outUV = inUV;
}