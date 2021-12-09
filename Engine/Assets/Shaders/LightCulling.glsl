// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Compute]
#version 460 core
#define TILE_SIZE 16

layout(set = 5, binding = 0) writeonly buffer VisibleLightIndicesBuffer
{
	int Indices[];

} sVisibleLightIndicesBuffer;

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
	sVisibleLightIndicesBuffer.Indices[0] = 2 + 2;
}