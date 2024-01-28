#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;
layout(set = 0, binding = 1) uniform CameraData
{
    float exposure;
} cameraData;

#include "hdr_utility.glsl"

//
// Tone-mapping operations from HDR to RGB - https://graphicrants.blogspot.com/2013/12/tone-mapping.html
//
// NOTE - Gamma correction is done automatically since render target has sRGB format
//

void main()
{
    vec3 hdrColor = texture(hdrTexture, inUV).rgb;

    // HDR tone-mapping, exposure is only set to 1.0 for now
    // if this changes make sure to consider exposure in the equation below
    //hdrColor = vec3(1.0) - exp(-hdrColor * cameraData.exposure);

    float luma = Luma(hdrColor);

    // Range is 1.0, so it's omitted from the equation altogether
    hdrColor = hdrColor / (1 + luma);

    outColor = vec4(hdrColor, 1.0);
}