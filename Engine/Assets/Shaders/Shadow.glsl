//SURGE:[Shader: Vertex]
#version 450
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBiTangent;
layout(location = 4) in vec2 aTexCoord;

layout(push_constant) uniform ShadowPushConstant
{
    mat4 LightSpaceTransform; // View Projection matrix from lights perspective
} pc;

// Per-instance transforms written by ShadowPass (all cascades back to back), gl_InstanceIndex includes firstInstance
layout(std430, set = 0, binding = 0) readonly buffer InstanceData
{
    mat4 Transforms[];
} uInstances;

void main()
{
    gl_Position = pc.LightSpaceTransform * uInstances.Transforms[gl_InstanceIndex] * vec4(aPosition, 1.0);
}

//SURGE:[Shader: Fragment]
#version 450

void main()
{
    // Intentionally left empty
}