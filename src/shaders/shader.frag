#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 lightDir = vec3(-1.0f, -1.0f, 0.0);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    // Calculate direct light contribution
    vec3 diffuseColor = dot(inNormal, -lightDir) * lightColor;

    vec4 sampleColor = texture(texSampler, inUV);

    outColor = sampleColor * vec4(diffuseColor, 1.0);
}