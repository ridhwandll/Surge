[SurgeShader: Vertex]
#version 460

layout(binding = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
} UBO;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = UBO.uProj * UBO.uView * UBO.uModel * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor, 1.0f);
}
