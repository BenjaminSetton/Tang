#version 450

layout(location = 0) in vec3 modelPos;

layout(location = 0) out vec3 localPos;

layout(set = 0, binding = 0) uniform ViewProjUBO
{
	mat4 view;
	mat4 proj;
} viewProjUBO;

void main()
{
	localPos = modelPos;

	mat4 rotationMatrix = mat4(mat3(viewProjUBO.view)); // We want to remove the translation component
	vec4 NDCPos = viewProjUBO.proj * rotationMatrix * vec4(localPos, 1.0);

	// Swizzling to ensure that the depth of the fragment always ends up at 1.0
	gl_Position = NDCPos.xyww;
}