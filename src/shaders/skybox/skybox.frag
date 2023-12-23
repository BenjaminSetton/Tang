#version 450

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraData
{
	float exposure;
} cameraData;

layout(set = 0, binding = 1) uniform samplerCube skyboxSampler;

void main()
{
	vec3 skyboxColor = texture(skyboxSampler, localPos).rgb;
	
	// HDR tone-mapping (exposure)
    skyboxColor = vec3(1.0) - exp(-skyboxColor * cameraData.exposure);

	 // NOTE - Gamma correction is not necessary since render target uses sRGB which automatically applies tonemapping

	outColor = vec4(skyboxColor, 1.0);
}