[SurgeShader: Vertex]
#version 460

layout(location = 0) in vec3 aPosition;

void main()
{
    gl_Position = vec4(aPosition, 1.0);
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants
{
    vec3 color;
} PushConstants;

void main()
{
    outColor = vec4(PushConstants.color, 1.0);
}
