#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;
layout(set = 0, binding = 1) uniform CameraData
{
    float exposure;
} cameraData;

void main()
{
    vec3 hdrColor = texture(hdrTexture, inUV).rgb;

    // HDR tone-mapping (exposure)
    hdrColor = vec3(1.0) - exp(-hdrColor * cameraData.exposure);

    // NOTE - Gamma correction is done automatically since render target has sRGB format

    outColor = vec4(hdrColor, 1.0);
}