
//
// Fragment code taken from LearnOpenGL: https://learnopengl.com/PBR/IBL/Diffuse-irradiance
//

#version 450

layout (location = 0) in vec3 localPos;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 2) uniform samplerCube skyboxCubemap;

const float PI = 3.14159265359;

void main()
{
	vec3 irradiance = vec3(0.0);  
    vec3 normal = normalize(localPos);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up         = normalize(cross(normal, right));

    float sampleDelta = 0.025;
    float sampleCount = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            vec3 colorSample = texture(skyboxCubemap, sampleVec).rgb * cos(theta) * sin(theta);

            // Tone-map the sample to avoid cube artifacts on irradiance map
            // NOTE - We are assuming a camera exposure of 1.0
            colorSample = vec3(1.0) - exp(-colorSample * 1.0);

            irradiance += colorSample;

            sampleCount++;
        }
    }

    outColor = vec4(PI * irradiance * (1.0 / float(sampleCount)), 1.0);
}