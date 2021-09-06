[SurgeShader: Vertex]
#version 460

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aPostio;
layout(location = 2) in float aPoitin;
layout(location = 3) in vec4 aositon;

/*
layout(binding = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
} UBO;
*/

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

void main()
{
    /*
    gl_Position = UBO.uProj * UBO.uView * UBO.uModel * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
    */
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}

[SurgeShader: Pixel]
#version 460

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 0.0, 1.0, 1.0);
}
