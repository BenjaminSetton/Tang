
const float PI = 3.14159265359;
const float EPSILON = 0.000001;

// Van der Corput sequence, used for Hammersley sequence. Mirrors a decimal binary representation around it's decimal point
float RadicalInverse_VDC(uint bits) 
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
    return vec2(float(i)/float(N), RadicalInverse_VDC(i));
}

// Fresnel (Schlick)
// NOTE - The roughness term accounts for the roughness around the edges of the surface
//        making the reflection weaker. This is needed since we started using diffuse IBL
//        (refer to diffuse IBL link at the top of the shader)
vec3 F(float HdotV, vec3 albedo, float metalness, float roughness)
{
    // baseReflectivity + ( 1 - baseReflectivity ) * ( 1 - dot( view, half ) )^5

    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, albedo, metalness);

    return baseReflectivity + ( max( vec3( 1.0 - roughness ), baseReflectivity ) - baseReflectivity ) * pow( ( 1.0 - HdotV ), 5.0 );
}

// Geometry (GGX - Schlick & Beckmann)
float G(float NdotV, float roughness)
{
    // dot( normal, view ) / ( dot( normal, view ) * ( 1 - K ) + K )
    
    // Using kIBL
    float K = ( roughness * roughness ) / 2.0;

    float funcNominator = NdotV;
    float funcDenominator = NdotV * ( 1.0 - K ) + K;

    return funcNominator / ( funcDenominator + EPSILON ); // Prevent division by 0
}

// NormalDistribution (GGX - Trowbridge & Reitz)
float D(float HdotN, float roughness)
{
    // roughness^2 / (PI * ( dot( normal, halfVector )^2 * ( roughness^2 - 1 ) + 1 )^2

    float funcNominator = pow( roughness, 2.0 );
    float funcDenominator = PI * pow( ( pow( HdotN, 2.0 ) * ( pow( roughness, 2.0 ) - 1.0 ) + 1.0 ), 2.0 );

    return funcNominator / ( funcDenominator + EPSILON ); // Prevent division by 0
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

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = G(NdotV, roughness);
    float ggx1 = G(NdotL, roughness);

    return ggx1 * ggx2;
}  