// Copyright (c) - SurgeTechnologies - All rights reserved
// SurgeEngine Light Culling Shader
// Based on
// - https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf
// - https://www.slideshare.net/takahiroharada/forward-34779335
// - https://wickedengine.net/2018/01/10/optimizing-tile-based-light-culling/
// - http://advances.realtimerendering.com/s2017/2017_Sig_Improved_Culling_final.pdf

[SurgeShader: Compute]
#version 460 core
#define TILE_SIZE 16

// Set 0 belongs to the renderer
layout(set = 0, binding = 0) uniform Camera
{
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    mat4 ViewProjectionMatrix;

} uCameraData;
layout(set = 0, binding = 1) uniform RendererData
{
    uint TilesCountX; // Forward+
    int ShowLightComplexity;
    float _Padding_1;
    float _Padding_2;
} uRendererData;

// Lights Data
struct PointLight
{
    vec3 Position;
    float Intensity;

    vec3 Color;
    float Radius; 

    float Falloff;
    int _Padding_;
    int _Padding_1;
    int _Padding_2;
};
struct DirectionalLight
{
    vec3 Direction;
    float Intensity;

    vec3 Color;
    float Size;
};
layout(set = 4, binding = 0) uniform Lights
{
    vec3 CameraPosition;
    int PointLightCount;

    PointLight PointLights[1024];
    DirectionalLight DirLight;

} uLights;

layout(push_constant) uniform ScreenData
{
    ivec2 ScreenSize;
} uScreenData;

layout(std430, set = 5, binding = 0) writeonly buffer VisibleLightIndicesBuffer
{
    int Indices[];

} sVisibleLightIndicesBuffer;
layout(set = 5, binding = 1) uniform sampler2D uPreDepthMap;

// Shared values between all the threads in the current work group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared vec4 frustumPlanes[6];
// Shared thread local storage(TLS) for visible indices, will be written out to the global buffer(sVisibleLightIndicesBuffer) at the end
shared int visibleLightIndices[1024];

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
    // Locations
    ivec2 location = ivec2(gl_GlobalInvocationID.xy);  // Global shader invocation location across the Dispatch call
    ivec2 itemID = ivec2(gl_LocalInvocationID.xy);     // Local shader invocation location inside the workgroup
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint index = tileID.y * tileNumber.x + tileID.x;

    // Initialize shared global values for depth and light count at the first invocation of this workgroup(only done at first thread invocation)
    if (gl_LocalInvocationIndex == 0)
    {
        minDepthInt = 0xFFFFFFFF;
        maxDepthInt = 0;
        visibleLightCount = 0;
    }
    barrier();

    // Calculate the minimum and maximum depth values (from the depth buffer) for this group's tile
    vec2 textureCoord = vec2(location) / uScreenData.ScreenSize;
    float depth = texture(uPreDepthMap, textureCoord).x;
    depth = uCameraData.ProjectionMatrix[3][2] / (depth + uCameraData.ProjectionMatrix[2][2]); // Linearize depth value

    // Convert depth to uint so we can do atomic min and max comparisons between the threads
    uint depthInt = floatBitsToUint(depth);
    atomicMin(minDepthInt, depthInt);
    atomicMax(maxDepthInt, depthInt);
    barrier();

    // One thread should calculate the frustum planes to be used for this tile (only done at first thread invocation)
    if (gl_LocalInvocationIndex == 0)
    {
        // Convert the min and max across the entire tile back to float
        float minDepth = uintBitsToFloat(minDepthInt);
        float maxDepth = uintBitsToFloat(maxDepthInt);

        // Steps based on tile sale
        vec2 negativeStep = (2.0 * vec2(tileID)) / vec2(tileNumber);
        vec2 positiveStep = (2.0 * vec2(tileID + ivec2(1, 1))) / vec2(tileNumber);

        // Set up starting values for planes using steps and min and max z values
        frustumPlanes[0] = vec4( 1.0,  0.0,  0.0,  1.0 - negativeStep.x); // Left
        frustumPlanes[1] = vec4(-1.0,  0.0,  0.0, -1.0 + positiveStep.x); // Right
        frustumPlanes[2] = vec4( 0.0,  1.0,  0.0,  1.0 - negativeStep.y); // Bottom
        frustumPlanes[3] = vec4( 0.0, -1.0,  0.0, -1.0 + positiveStep.y); // Top
        frustumPlanes[4] = vec4( 0.0,  0.0, -1.0, -minDepth); // Near
        frustumPlanes[5] = vec4( 0.0,  0.0,  1.0,  maxDepth); // Far

        // Transform the first four planes
        for (uint i = 0; i < 4; i++)
        {
            frustumPlanes[i] *= uCameraData.ViewProjectionMatrix;
            frustumPlanes[i] /= length(frustumPlanes[i].xyz);
        }

        // Transform the depth planes
        frustumPlanes[4] *= uCameraData.ViewMatrix;
        frustumPlanes[4] /= length(frustumPlanes[4].xyz);
        frustumPlanes[5] *= uCameraData.ViewMatrix;
        frustumPlanes[5] /= length(frustumPlanes[5].xyz);
    }
    barrier();

    // Cull lights
    // Parallelize the threads against the lights now
    // Can handle 256(TILE_SIZE * TILE_SIZE) simultaniously
    uint threadCount = TILE_SIZE * TILE_SIZE;
    uint passCount = (uLights.PointLightCount + threadCount - 1) / threadCount;
    for (uint i = 0; i < passCount; i++)
    {
        // Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
        uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
        if (lightIndex >= uLights.PointLightCount)
            break;

        vec4 position = vec4(uLights.PointLights[lightIndex].Position, 1.0);
        float radius = uLights.PointLights[lightIndex].Radius;
        radius += radius * 0.3;

        // Check if light radius is in frustum
        float distance = 0.0;
        for (uint j = 0; j < 6; j++)
        {
            distance = dot(position, frustumPlanes[j]) + radius;
            if (distance <= 0.0) // No intersection
                break;
        }

        // If greater than zero, then it is a visible light
        if (distance > 0.0)
        {
            // Add index to the shared array of visible indices
            uint offset = atomicAdd(visibleLightCount, 1);
            visibleLightIndices[offset] = int(lightIndex); // Add to Thread Local Storage(TLS)
        }
    }
    barrier();

    // One thread should fill the global light buffer
    if (gl_LocalInvocationIndex == 0)
    {
        uint offset = index * 1024; // Determine position in global buffer
        for (uint i = 0; i < visibleLightCount; i++)
        {
            sVisibleLightIndicesBuffer.Indices[offset + i] = visibleLightIndices[i];
        }

        if (visibleLightCount != 1024)
        {
            // Unless we have totally filled the entire array, mark it's end with -1
            // Final shader step will use this to determine where to stop (without having to pass the light count)
            sVisibleLightIndicesBuffer.Indices[offset + visibleLightCount] = -1;
        }
    }
}
