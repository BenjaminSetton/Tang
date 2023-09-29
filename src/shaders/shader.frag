#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

    // Light variables
    vec3 lightDir = normalize(vec3(0.0, -1.0, 0.0));
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    // Surface variables
    vec3 surfaceColor = vec3(0.9, 0.4, 0.4);

    vec3 ambientColor = vec3(0.01, 0.01, 0.01);

    // Calculate direct light contribution
    float diffuseFactor = clamp(dot(normalize(inNormal), -lightDir), 0.0f, 1.0f);
    vec3 diffuseColor = surfaceColor * diffuseFactor;

    //vec4 sampleColor = texture(texSampler, inUV);

    outColor = vec4((diffuseColor + ambientColor), 1.0) * vec4(1.0);

    // Diffuse
    // diffuseColor = clamp( dot(-L * N) * (lightColor * lightIntensity) )
    // sampleColor = texture(sampler, inputUV)
    // finalColor = (ambientColor + diffuseColor) * sampleColor 
}