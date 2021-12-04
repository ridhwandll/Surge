// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Vertex]
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;    // Unused
layout(location = 2) in vec3 aTangent;   // Unused
layout(location = 3) in vec3 aBiTangent; // Unused
layout(location = 4) in vec2 aTexCoord;  // Unused

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    mat4 ViewProjectionMatrix;

} uMesh;

void main()
{
    gl_Position = uMesh.ViewProjectionMatrix * uMesh.Transform * vec4(aPosition, 1.0);
}

[SurgeShader: Pixel]
#version 460 core

void main()
{
}