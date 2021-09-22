[SurgeShader: Vertex]
#version 460

layout(location = 0) in vec3 aPosition;

layout(push_constant) uniform PushConstants
{
    mat4 ViewProjection;
    mat4 Transform;
} uFrameData;

void main()
{
    gl_Position = uFrameData.ViewProjection * uFrameData.Transform * vec4(aPosition, 1.0);
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 0.1, 0.1, 1.0);
}
