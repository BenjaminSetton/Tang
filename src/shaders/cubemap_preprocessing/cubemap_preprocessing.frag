#version 450

//
// This shader essentially takes the projection-space (interpolated) position
// and maps it from spherical coordinates (this is how the information is
// stored in the equirectangular map) to cartesian coordinates so we can
// sample the 2D texture correctly. Again, this process is better explained
// in LearnOpenGL:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
//

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 localPos;

layout(set = 0, binding = 1) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    outColor = vec4(color, 1.0);
}