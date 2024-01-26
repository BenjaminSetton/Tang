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

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Returns a low-discrepancy sample 'i' over the total sample size 'N'
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// Approximates the surface area of microfacets aligned exactly to the halfway vector,
// considering the roughness
float DistributionGGX(float NdotH, float roughness)
{
    float roughness2     = roughness * roughness;
    float NdotH2 = NdotH * NdotH;
	
    float nom    = roughness2;
    float denom  = (NdotH2 * (roughness2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
	
    return nom / denom;
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
    // Spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // Tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  

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
            float D   = DistributionGGX(NdotH, roughness.value);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness.value == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    outColor = vec4(prefilteredColor, 1.0);
}  