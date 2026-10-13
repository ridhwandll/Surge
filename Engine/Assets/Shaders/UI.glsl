//SURGE:[Shader: Vertex]
#version 450

// Screen space UI: sprites and MSDF text share one shader/pipeline so they batch together and keep their submission (hierarchy) order

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inTextureIndex; // bits 0-7: texture slot, bits 8-23: MSDF px range * 256, bit 31: MSDF text glyph

layout(push_constant) uniform PushConstants
{
    mat4 ViewProjection; // Orthographic, (0,0) = top left
} uFrame;

layout(location = 0) flat out uint outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) flat out uint outTextureIndex;

void main()
{
    gl_Position = uFrame.ViewProjection * vec4(inPosition, 1.0);
    outColor = inColor;
    outUV = inUV;
    outTextureIndex = inTextureIndex;
}

//SURGE:[Shader: Fragment]
#version 450

layout(set = 0, binding = 0) uniform sampler2D uTextures[16];

layout(location = 0) flat in uint inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) flat in uint inTextureIndex;

layout(location = 0) out vec4 outColor;

const uint TEXT_FLAG = 0x80000000u;

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    // UI is drawn after post processing, colors are already display (gamma) space, no linearization here
    vec4 color = unpackUnorm4x8(inColor);
    uint slot = inTextureIndex & 0xFFu;

    // Derivatives before branching (the flag is flat per primitive, but keep it obviously safe)
    vec2 uvDx = dFdx(inUV);
    vec2 uvDy = dFdy(inUV);

    if((inTextureIndex & TEXT_FLAG) == 0u)
    {
        outColor = texture(uTextures[slot], inUV) * color;

        // Rounded rect: bits 8-23 hold the corner radius in quarter pixels. The quad is axis aligned and its UVs map it linearly,
        // so the rect size in pixels is 1 / (UV change per pixel), this also holds for quads clipped on the CPU (same mapping)
        uint radiusBits = (inTextureIndex >> 8) & 0xFFFFu;
        if(radiusBits != 0u)
        {
            vec2 uvPerPixel = vec2(abs(uvDx.x) + abs(uvDy.x), abs(uvDx.y) + abs(uvDy.y));
            vec2 halfSize = 0.5 / max(uvPerPixel, vec2(1e-6));
            vec2 p = (inUV - 0.5) * 2.0 * halfSize; // Pixels from the rect center
            float radius = min(float(radiusBits) * 0.25, min(halfSize.x, halfSize.y));

            // Signed distance to a rounded box, negative inside
            vec2 q = abs(p) - halfSize + radius;
            float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
            outColor.a *= clamp(0.5 - dist, 0.0, 1.0); // 1 pixel wide anti-aliased edge
        }
        return;
    }

    // MSDF text
    float pxRange = float((inTextureIndex >> 8) & 0xFFFFu) / 256.0;
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(uTextures[slot], 0));
    vec2 screenTexSize = inversesqrt(uvDx * uvDx + uvDy * uvDy);
    float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);

    vec3 msd = texture(uTextures[slot], inUV).rgb;
    float sd = Median(msd.r, msd.g, msd.b);
    float opacity = clamp(screenPxRange * (sd - 0.5) + 0.5, 0.0, 1.0);
    if(opacity < 0.001)
        discard;

    outColor = vec4(color.rgb, color.a * opacity); // Color alpha applies to text too (fading labels)
}
