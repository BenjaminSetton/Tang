#version 450

layout(location = 0) in vec3 modelPos;

layout(location = 0) out vec3 localPos;

layout(set = 2, binding = 0) uniform mat4 viewMatrix;
layout(set = 2, binding = 1) uniform mat4 projMatrix;

void main()
{
	localPos = modelPos;

	mat4 rotationMatrix = mat4(mat3(viewMatrix)); // We want to remove the translation component
	vec4 NDCPos = projMatrix * rotationMatrix * vec4(localPos, 1.0);

	// Swizzling to ensure that the depth of the fragment always ends up at 1.0
	gl_Position = NDCPos.xyww;
}