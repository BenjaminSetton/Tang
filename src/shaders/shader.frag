#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

    // Light variables
    vec3 lightDir = normalize(vec3(-0.5f, 1.0f, 0.0f));
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    // Surface variables
    vec3 surfaceColor = vec3(0.9, 0.4, 0.4);

    float ambientIntensity = 0.01f;

    // Calculate direct light contribution
    float contribution = clamp(dot(inNormal, -lightDir), 0.0f, 1.0f);
    vec3 diffuseColor = contribution * lightColor * surfaceColor;

    //vec4 sampleColor = texture(texSampler, inUV);

    outColor = vec4(diffuseColor, 1.0) + ambientIntensity;
}