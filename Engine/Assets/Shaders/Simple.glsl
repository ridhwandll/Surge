[SurgeShader: Vertex]
#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aPostio;
layout(location = 2) in float aPoitin;
layout(location = 3) in vec4 aositon;

layout(binding = 0, set = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
} UBO;

layout(binding = 1, set = 0) uniform sampler2D exampleTexture;
layout(binding = 0, set = 1) uniform sampler2D shadowTexture;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

layout(push_constant) uniform Constants
{
    mat4 viewInverse;
    mat4 projInverse;
    int  rayAccumulation;
} ps_Camera;

void main()
{
    gl_Position = UBO.uProj * UBO.uView * UBO.uModel * vec4(positions[gl_VertexIndex], 0.0, 1.0);
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
} UBO;

layout(binding = 1, set = 0) uniform sampler2D exampleTexture;

layout(binding = 2, set = 0) buffer StorageBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
} SBO;

layout(push_constant) uniform Constants
{
    mat4 viewInverse;
    mat4 projInverse;
    int  rayAccumulation;
} ps_Camera;

void main()
{
    outColor = vec4(1.0, 0.0, 1.0, 1.0);
}
