
// Resources:
// PBR (Victor Gordan) - https://www.youtube.com/watch?v=RRE-F57fbXw&t=10s
// PBR explanation (OGL Dev) - https://www.youtube.com/watch?v=XK_p2MxGBQs&t=126s
// Diffuse IBL (LearnOpenGL) - https://learnopengl.com/PBR/IBL/Diffuse-irradiance

// Simplified rendering equation:
// vec3 finalColor = ( DiffuseBRDF + SpecularBRDF ) * LightIntensity * NdotL

// Explanation of the components of the equation above, assuming the Cook-Torrance BRDF function:
// [ DiffuseBRDF ]
// kD * Lambert / PI

// [ SpecularBRDF ]
// kS * CookTorrance()

// [ LightIntensity ]
// N

// [NdotL]
// dot( surfaceNormal, lightVector )

// Explanation of [ DiffuseBRDF ]:
// - kD and kS represent the ratio between the refraction and reflection components. Their sum must always equal 1, otherwise 
//   the law of conservation of energy is violated and this isn't a PBR shader anymore.
// - Lambert represents the material color (retrieved from diffuse texture). This means that the diffuse BRDF value is only applied
//   to dielectrics, and NOT metals!

// Explanation of [ SpecularBRDF ]:
// - The CookTorrance() function is, itself, made up of a separate equation, namely: (D * G * F) / 4 * dot(normal, light) * dot(normal, view).
//   D (normal distribution function), G (geometry function), and F (fresnel function) are all functions as well
// - D represents the number of microfacets that reflect light back at the viewer. More specifically, it tells us how many microfacets have their
//   half-vector aligned with the surface normal. If the half-vector is completely aligned with the normal, this means that ALL the light is being
//   reflected towards the viewer.
// - The function we use for D is GGX (Trowbridge & Reitz), given below:
//      roughness^2 / (PI * ( dot( normal, halfVector )^2 * ( roughness^2 - 1 ) + 1 )^2
//   When this function is 0 it implies an ideal smooth surface, while a value of 1 implies maximum roughness
// - G approximates the self-shadowing factor of microfacets. It tells us the probability that any given microfacet with it's normal is
//   visible to both the light source and the viewer
// - The function we use for G is GGX (Schlick & Beckmann), given below:
//      dot( normal, view ) / ( dot( normal, view ) * ( 1 - K ) + K )
//      where K = ( roughness + 1 )^2 / 8
// - F approximates the fresnel effect, where light bouncing off a lower degree of incidence will be reflected more often than light reflecting with
//   a higher degree of incidence (aka closer to the surface's normal).
// - The function we use for F is the Schlick approximation, given below:
//      baseReflectivity + ( 1 - baseReflectivity ) * ( 1 - dot( view, half ) )^5
//   NOTE: The base reflectivity changes over the visible spectrum, which is why it's represented as an RGB value instead of a 
//         float like most other values in the PBR equation. Common practice is to use a value of 0.04 (across all RGB values)
//         for all dielectric materials, but to use the real base reflectivity for metals (which can simply be looked up)


#version 450

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in mat3 inTBN;

layout(set = 0, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D metallicSampler;
layout(set = 0, binding = 3) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 4) uniform sampler2D lightmapSampler;
layout(set = 0, binding = 5) uniform samplerCube irradianceMap;
layout(set = 0, binding = 7) uniform sampler2D BRDFLUT;
layout(set = 0, binding = 6) uniform samplerCube prefilterMap;

layout(set = 2, binding = 1) uniform CameraData {
    vec4 position;
    float exposure;
    vec3 padding1;
    vec4 padding2;
    vec4 padding3;
} cameraData;

layout(location = 0) out vec4 outColor;

const float MAX_REFLECTION_LOD = 4.0;

#include "pbr_utility.glsl"

// NOTE - The light vector must be pointing TOWARDS the light source
void main() 
{
    // NORMAL MAP
    vec3 normal = texture(normalSampler, inUV).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize( inTBN * normal );
    ////

    // BASE VECTORS
    vec3 cameraPos = cameraData.position.xyz;
    vec3 light = -normalize(vec3(0.4, -0.75, 1.0));
    vec3 view = normalize(cameraPos - inWorldPosition);
    vec3 halfVector = normalize(light + view);
    vec3 albedo = texture(diffuseSampler, inUV).rgb;
    vec3 reflection = reflect(-view, normal);
    ////

    // BASE DOT PRODUCTS + TEXTURE SAMPLES
    float NdotV = max(dot(normal, view), 0.0);
    float NdotL = max(dot(normal, light), 0.0);
    float HdotV = max(dot(halfVector, view), 0.0);
    float HdotN = max(dot(halfVector, normal), 0.0);
    float lightIntensity = 1.0; // NOTE - This is only for point / spotlights, which we do not support right now
    float metalness = texture(metallicSampler, inUV).b;
    float roughness = texture(roughnessSampler, inUV).g;
    ////

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    vec3 pbrColor = vec3(0.0);
    // BRDF CALCULATION
    {
        float D = D_GGX(HdotN, roughness);
        float G = G_Smith(normal, view, light, roughness);
        vec3 F = F_Schlick(HdotV, F0);

        //light = light * 0.5 + 0.5;
        //outColor = vec4(inWorldPosition, 1.0);
        //return;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness; // Kill diffuse component if we're dealing with a metal

        vec3 numerator = D * G * F;
        float denominator = ( 4.0 * NdotL * NdotV ) + EPSILON;
        vec3 specularBRDF = numerator / denominator;

        vec3 diffuseBRDF = kD * albedo / PI;

        pbrColor = ( diffuseBRDF + specularBRDF ) * lightIntensity * NdotL;
    }
    ////

    vec3 ambient = vec3(0.0);
    // AMBIENT CALCULATION (IBL)
    {
        vec3 F = F_Roughness(NdotV, F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metalness;

        vec3 irradiance = texture(irradianceMap, normal).rgb;
        vec3 diffuse = irradiance * albedo;

        vec3 prefilteredColor = textureLod(prefilterMap, reflection, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 BRDFSample = texture(BRDFLUT, vec2(NdotV, roughness)).rg;
        vec3 specular = prefilteredColor * (F * BRDFSample.x + BRDFSample.y);

        ambient = (kD * diffuse + specular) * 1.0; // Hard-coding the ambient occlusion term to 1.0 for now
    }
    ////

    pbrColor += ambient;

    outColor = vec4( pbrColor, 1.0 );
}