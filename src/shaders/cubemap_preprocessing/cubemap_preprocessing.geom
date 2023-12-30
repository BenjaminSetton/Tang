#version 450

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 1) uniform CubemapLayer
{
    int face;
} cubemapLayer;

layout (location = 0) in vec3 in_localPos[];

layout (location = 0) out vec3 out_localPos;

void main() 
{
    for(int i = 0; i < gl_in.length(); i++)
    {
        gl_Position = gl_in[i].gl_Position;
        gl_Layer = cubemapLayer.face;
        out_localPos = in_localPos[i];
        EmitVertex();
    }

    EndPrimitive();
}