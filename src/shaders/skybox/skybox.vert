#version 450

layout(location = 0) in vec3 modelPos;

layout(location = 0) out vec3 localPos;

layout(set = 1, binding = 0) uniform ViewUBO
{
	mat4 viewMatrix;
} viewUBO;

layout(set = 1, binding = 1) uniform ProjUBO
{
	mat4 projMatrix;
} projUBO;

void main()
{

	mat4 rotationMatrix = mat4(mat3(viewUBO.viewMatrix)); // We want to remove the translation component

	localPos = (rotationMatrix * vec4(modelPos, 0.0f)).xyz;

	vec4 NDCPos = projUBO.projMatrix * rotationMatrix * vec4(localPos, 1.0);

	// Swizzling to ensure that the depth of the fragment always ends up at 1.0
	gl_Position = NDCPos.xyww;
}