
// Resources:
// PBR (Victor Gordan) - https://www.youtube.com/watch?v=RRE-F57fbXw&t=10s
// PBR explanation (OGL Dev) - https://www.youtube.com/watch?v=XK_p2MxGBQs&t=126s

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
// - Lambert represents the material color (retrieved from diffuse texture??)

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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D metallicSampler;
layout(set = 0, binding = 3) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 4) uniform sampler2D lightmapSampler;

layout(set = 2, binding = 1) uniform CameraData {
    vec4 position;
    vec4 padding1;
    vec4 padding2;
    vec4 padding3;
} cameraData;

layout(location = 0) out vec4 outColor;

void main() {

    // TODO - PBR shader

    outColor = texture(diffuseSampler, inUV);
}