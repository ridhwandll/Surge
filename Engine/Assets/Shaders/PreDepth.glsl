// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Vertex]
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;    // Unused
layout(location = 2) in vec3 aTangent;   // Unused
layout(location = 3) in vec3 aBiTangent; // Unused
layout(location = 4) in vec2 aTexCoord;  // Unused

// Set 0 belongs to the renderer
//layout(set = 0, binding = 0) uniform Camera
//{
//    mat4 ViewMatrix;
//    mat4 ProjectionMatrix;
//    mat4 ViewProjectionMatrix;
//
//} uCameraData;
//layout(set = 0, binding = 1) uniform RendererData
//{
//    uint TilesCountX; // Forward+
//    int ShowLightComplexity;
//    float _Padding_1;
//    float _Padding_2;
//} uRendererData;

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    mat4 ViewProjectionMatrix;

} uMesh;
//layout(location = 0) out float vLinearDepth;

void main()
{
    vec4 worldPosition = uMesh.Transform * vec4(aPosition, 1.0);
    //vLinearDepth = -(uCameraData.ViewMatrix * worldPosition).z;

    gl_Position = uMesh.ViewProjectionMatrix * worldPosition;
}

[SurgeShader: Pixel]
#version 460 core

//layout(location = 0) out vec4 outputLinearDepth;
//layout(location = 0) in float vLinearDepth;

void main()
{
    //outputLinearDepth = vec4(vLinearDepth, 0.0, 0.0, 1.0);
}