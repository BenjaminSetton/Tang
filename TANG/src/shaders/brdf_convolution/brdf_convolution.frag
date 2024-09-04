
//
// Code adapted from https://learnopengl.com/PBR/IBL/Specular-IBL
//

#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec2 outColor;

#include "pbr_utility.glsl"

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float HdotV = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = G_Smith(N, V, L, roughness);
            float G_Vis = (G * HdotV) / (NdotH * NdotV);
            float Fc = pow(1.0 - HdotV, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() 
{
    vec2 integratedBRDF = IntegrateBRDF(inUV.x, inUV.y);
    outColor = integratedBRDF;
}