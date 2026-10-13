//SURGE:[Shader: Vertex]
#version 450

// Particles are pulled from storage buffers (ParticlePass), no vertex input: 6 vertices per particle
struct ParticleRecord
{
    vec3 Position;         // World space
    uint Size;             // half2
    uint RotationFrame;    // half rotation | uint16 frame, 8.8 fixed point
    uint Color;            // RGBA8, sRGB
    uint VelocityXY;       // half2
    uint VelocityZEmitter; // half velocity z | uint16 emitter index
};

struct EmitterData
{
    uint Mode;
    uint Columns;
    uint Rows;
    uint FrameBlending;
    float AdditiveAmount;
    float EmissiveIntensity;
    float MaxScreenSize;
    float StretchSpeedScale;
    float StretchLengthScale;
    float _pad0, _pad1, _pad2;
};

layout(std140, set = 0, binding = 0) uniform FrameUBO
{
    mat4 View;
    mat4 ViewProjection;
    mat4 InverseViewProjection;
    vec3 CameraPos;
    float _pad;
} uFrame;

layout(std430, set = 0, binding = 1) readonly buffer Particles
{
    ParticleRecord Records[];
} uParticles;

layout(std430, set = 0, binding = 2) readonly buffer Emitters
{
    EmitterData Data[];
} uEmitters;

layout(push_constant) uniform PushConstants
{
    float ProjectionScaleY; // Projection[1][1]
} uPush;

layout(location = 0) out vec2 vUV0;
layout(location = 1) out vec2 vUV1;
layout(location = 2) out vec4 vColor;
layout(location = 3) flat out vec3 vParams; // Frame blend, additive amount, emissive intensity

const uint MODE_BILLBOARD = 0u;
const uint MODE_STRETCHED = 1u;
const uint MODE_HORIZONTAL = 2u;
const uint MODE_VERTICAL = 3u;

const vec2 CORNERS[6] = vec2[](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(0.5, 0.5),
                               vec2(-0.5, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));

vec2 FrameUV(vec2 uv, uint frame, uint columns, uint rows)
{
    uvec2 cell = uvec2(frame % columns, frame / columns);
    return (uv + vec2(cell)) / vec2(columns, rows);
}

void main()
{
    uint particleIndex = uint(gl_VertexIndex) / 6u;
    vec2 corner = CORNERS[uint(gl_VertexIndex) % 6u];

    ParticleRecord particle = uParticles.Records[particleIndex];
    EmitterData emitter = uEmitters.Data[particle.VelocityZEmitter >> 16];

    vec2 size = unpackHalf2x16(particle.Size);
    float rotation = unpackHalf2x16(particle.RotationFrame).x;
    vec3 velocity = vec3(unpackHalf2x16(particle.VelocityXY), unpackHalf2x16(particle.VelocityZEmitter).x);

    vec3 cameraRight = vec3(uFrame.View[0][0], uFrame.View[1][0], uFrame.View[2][0]);
    vec3 cameraUp = vec3(uFrame.View[0][1], uFrame.View[1][1], uFrame.View[2][1]);
    vec3 toCamera = uFrame.CameraPos - particle.Position;

    // Close to the camera a particle could cover the whole screen, cap its height to a fraction of the screen (overdraw)
    vec4 centerClip = uFrame.ViewProjection * vec4(particle.Position, 1.0);
    float screenFraction = size.y * uPush.ProjectionScaleY / max(centerClip.w, 1e-4) * 0.5;
    if(screenFraction > emitter.MaxScreenSize)
        size *= emitter.MaxScreenSize / screenFraction;

    vec3 right;
    vec3 up;
    if(emitter.Mode == MODE_STRETCHED)
    {
        float speed = length(velocity);
        vec3 axis = speed > 1e-4 ? velocity / speed : cameraUp;
        vec3 side = cross(axis, toCamera);
        side = dot(side, side) > 1e-8 ? normalize(side) : cameraRight;
        right = side * size.x;
        up = axis * (size.y * emitter.StretchLengthScale + speed * emitter.StretchSpeedScale);
    }
    else
    {
        vec3 baseRight = cameraRight;
        vec3 baseUp = cameraUp;
        if(emitter.Mode == MODE_HORIZONTAL)
        {
            baseRight = vec3(1.0, 0.0, 0.0);
            baseUp = vec3(0.0, 0.0, -1.0);
        }
        else if(emitter.Mode == MODE_VERTICAL)
        {
            baseUp = vec3(0.0, 1.0, 0.0);
            vec3 side = cross(baseUp, toCamera);
            baseRight = dot(side, side) > 1e-8 ? normalize(side) : cameraRight;
        }

        float s = sin(rotation);
        float c = cos(rotation);
        right = (baseRight * c + baseUp * s) * size.x;
        up = (baseUp * c - baseRight * s) * size.y;
    }

    vec3 worldPosition = particle.Position + right * corner.x + up * corner.y;
    gl_Position = uFrame.ViewProjection * vec4(worldPosition, 1.0);

    // Flipbook, frame is 8.8 fixed point: the fraction blends into the next frame
    vec2 uv = vec2(corner.x + 0.5, 0.5 - corner.y);
    uint frameCount = emitter.Columns * emitter.Rows;
    float frame = float(particle.RotationFrame >> 16) / 256.0;
    uint frame0 = uint(frame) % frameCount;
    uint frame1 = (frame0 + 1u) % frameCount;
    vUV0 = FrameUV(uv, frame0, emitter.Columns, emitter.Rows);
    vUV1 = FrameUV(uv, frame1, emitter.Columns, emitter.Rows);

    vColor = unpackUnorm4x8(particle.Color);
    vParams = vec3(emitter.FrameBlending != 0u ? fract(frame) : 0.0, emitter.AdditiveAmount, emitter.EmissiveIntensity);
}

//SURGE:[Shader: Fragment]
#version 450

layout(set = 1, binding = 0) uniform sampler2D uTexture;

layout(location = 0) in vec2 vUV0;
layout(location = 1) in vec2 vUV1;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in vec3 vParams;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 texel = texture(uTexture, vUV0);
    if(vParams.x > 0.0)
        texel = mix(texel, texture(uTexture, vUV1), vParams.x);

    vec4 color = vColor;
    color.rgb = pow(color.rgb, vec3(2.2)); // Authored colors are sRGB, the scene is linear (gamma is applied in PostProcess)
    color *= texel;

    // Premultiplied alpha (ONE, ONE_MINUS_SRC_ALPHA): additive particles keep their color but stop occluding what is behind them
    outColor = vec4(color.rgb * color.a * vParams.z, color.a * (1.0 - vParams.y));
}
