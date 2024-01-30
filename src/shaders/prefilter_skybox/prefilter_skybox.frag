//
// Shader code taken from and explained in: https://learnopengl.com/PBR/IBL/Specular-IBL
//

#version 450

layout(location = 0) in vec3 inLocalPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform samplerCube environmentMap;
layout(set = 1, binding = 0) uniform Roughness
{
    float value;
} roughness;

#include "pbr_utility.glsl"
#include "hdr_utility.glsl"

void main()
{
    // Note on (N = R = V) below:
    //
    // As we don't know beforehand the view direction when convoluting the environment map, Epic Games makes 
    // a further approximation by assuming the view direction (and thus the specular reflection direction) 
    // to be equal to the output sample direction w0.
    // This means we don't need to worry about the view direction when convoluting the pre-filter map, but
    // the compromise is that we don't get nice grazing specular reflections (see https://learnopengl.com/PBR/IBL/Specular-IBL
    // for reference images)

    vec3 N = normalize(inLocalPos);
    vec3 R = N;
    vec3 V = R;

    // Isn't this always 1.0 when N = R = V??
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    vec3 eyeVec = vec3( sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV );

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness.value);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = clamp(H.z, 0.0, 1.0);
        float HdotV = clamp(dot(H, eyeVec), 0.0, 1.0);

        if(NdotL > 0.0) // Is this check even necessary if we're clamping beforehand?
        {
            float D   = D_GGX(NdotH, roughness.value);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness.value == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

            vec3 environmentSample = textureLod(environmentMap, L, mipLevel).rgb;
            prefilteredColor += environmentSample * NdotL;

            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    outColor = vec4(prefilteredColor, 1.0);
}  