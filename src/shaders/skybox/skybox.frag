#version 450

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform CameraData
{
	float exposure;
} cameraData;

layout(set = 0, binding = 0) uniform samplerCube skyboxSampler;

void main()
{
	vec3 skyboxColor = texture(skyboxSampler, localPos).rgb;

	outColor = vec4(skyboxColor, 1.0);
}