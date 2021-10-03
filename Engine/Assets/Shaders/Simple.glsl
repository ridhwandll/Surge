[SurgeShader: Vertex]
#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 ViewProjection;
    mat4 Transform;
} uFrameData;

layout(location = 0) out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = uFrameData.ViewProjection * uFrameData.Transform * vec4(aPosition, 1.0);
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 vTexCoord;

layout(binding = 0, set = 0) uniform sampler2D u_Texture;

void main()
{
    vec3 texColor = texture(u_Texture, vTexCoord).xyz;
    outColor = vec4(texColor, 1.0);
}