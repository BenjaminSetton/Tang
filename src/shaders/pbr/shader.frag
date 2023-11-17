#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform CameraData {
    vec4 position;
    vec4 padding1;
    vec4 padding2;
    vec4 padding3;
} cameraData;

layout(location = 0) out vec4 outColor;

void main() {

    // Light constants
    vec3 lightDir = normalize(vec3(-0.75, -1.0, 0.0));
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float specularPower = 64.0;
    vec3 surfaceColor = vec3(0.9, 0.4, 0.4);
    vec3 ambientColor = vec3(0.01, 0.01, 0.01);
    vec3 cameraPos = cameraData.position.xyz;
    vec3 surfaceNormal = normalize(inNormal);
    float lightContribution = clamp(dot(surfaceNormal, -lightDir), 0.0, 1.0);


    // Calculate direct light contribution
    vec3 diffuseColor = surfaceColor * lightContribution;

    // Reflect the light along the pixel normal
    // Calculate pixel pos - camera pos to get a vector looking at the pixel
    // The closer the dot between the reflected light and the looking vector is to 1, the more light we should receive
    // Take the incoming light received to a high exponent to mimic a specular highlight. Higher exponent = tighter specular, lower exponent = looser specular
    
    // Calculate specular highlight
    vec3 cameraToFragment = normalize(inPosition - cameraPos);
    vec3 reflectedLight = normalize(reflect(-lightDir, surfaceNormal));
    float specularIntensity = clamp(dot(cameraToFragment, reflectedLight), 0.0, 1.0);
    vec3 specularColor = vec3(1.0) * pow(specularIntensity, specularPower) * lightContribution;

    //vec4 sampleColor = texture(texSampler, inUV);

    outColor = vec4((diffuseColor + ambientColor + specularColor), 1.0);
}