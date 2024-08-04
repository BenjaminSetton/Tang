
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
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    // F0 + ( 1 - F0 ) * ( 1 - dot( view, half ) )^5
    // where F0 represents the base reflectivity

    return F0 + ( 1.0 - F0 ) * pow( 1.0 - cosTheta, 8.0 );
}

// Fresnel (Schlick + roughness)
// NOTE - The roughness term accounts for the roughness around the edges of the surface
//        making the reflection weaker. This is needed since we started using diffuse IBL
//        (refer to diffuse IBL link at the top of the shader)
vec3 F_Roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( ( 1.0 - cosTheta ), 5.0 );
}

// Geometry (GGX - Schlick & Beckmann)
float G_GGX(float NdotV, float roughness)
{
    // Using kIBL
    float a = roughness;
    float K = ( a * a ) / 2.0;

    float funcNominator = NdotV;
    float funcDenominator = NdotV * ( 1.0 - K ) + K;

    return funcNominator / funcDenominator;
}

// Geometry (Smith)
float G_Smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = G_GGX(NdotV, roughness);
    float ggx1 = G_GGX(NdotL, roughness);

    return ggx1 * ggx2;
}  

// Normal Distribution (GGX - Trowbridge & Reitz)
float D_GGX(float HdotN, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float funcNominator = a2;
    float funcDenominator = ( pow( HdotN, 2.0 ) * ( a2 - 1.0 ) + 1.0 );
    funcDenominator = PI * pow( funcDenominator, 2.0 );

    return funcNominator / ( funcDenominator + EPSILON ); // Prevent division by 0
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