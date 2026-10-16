//SURGE:[Shader: Fragment]
#version 450
const int MAX_CASCADE_COUNT = 3;

struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 BiTangent;
    vec3 WorldPos;
    float ViewDepth;  // Positive view space depth, picks the shadow cascade
    vec3 ShadowCoord; // Cascade 0 light space position, the other cascades are a scale + offset of it
};
layout(location = 0) in VertexOutput vInput;
layout(location = 0) out vec4 FinalColor;

struct Light
{
    vec4 PositionType;   // xyz = pos/dir, w = type (0 = dir, 1 = point)
    vec3 Color;
    float Intensity;
    float Radius;
    float Falloff;
    float _pad1, _pad2;
};

// Push Constants
layout(push_constant) uniform PushConstants
{
    uint LightCount; // Already culled + capped on the CPU (FrameBlackboard::Lighting)
} uMesh;

// Set 0: Frame + Lights
layout(std140, set = 0, binding = 0) uniform FrameUBO
{
    mat4 View;
    mat4 ViewProjection;
    mat4 InverseViewProjection;
    vec3 CameraPos;
    float _pad;
} uFrame;

layout(std140, set = 0, binding = 1) readonly buffer Lights
{
    vec3 SkyAmbient;
    float _pad1;
    vec3 HorizonAmbient;
    float _pad2;
    vec3 GroundAmbient;
    float _pad3;

    Light Lights[256];
} uLights;

// Set 1: Materials
layout(set = 1, binding = 0) uniform Material
{
    vec3 Albedo;
    float Metallic;
    float Roughness;
    float Reflectance;
    int UseAlbedoMap;
    int UseNormalMap;
    int UseMetallicMap;
    int UseRoughnessMap;
    int UseEmissionMap;
    vec3 Emission;
    float EmissionIntensity;
} uMaterial;

layout(set = 1, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 1, binding = 2) uniform sampler2D NormalMap;
layout(set = 1, binding = 3) uniform sampler2D RoughnessMetallicMap;
layout(set = 1, binding = 4) uniform sampler2D EmissionMap;

// Set 2: Shadows, one D16 atlas with the cascades side by side
layout(set = 2, binding = 0) uniform sampler2DShadow uShadowAtlas;
layout(std140, set = 2, binding = 1) uniform ShadowUBO
{
    mat4 LightSpaceMatrix;                 // World -> cascade 0
    vec4 CascadeScale[MAX_CASCADE_COUNT];  // xyz: cascade 0 -> cascade i, w: world size of one shadow texel
    vec4 CascadeOffset[MAX_CASCADE_COUNT];
    vec4 CascadeEnds;                      // Positive view depth
    vec2 AtlasTexelSize;
    float NormalBias;                      // In shadow texels
    float CascadeBlend;
    float FadeStart;
    float Distance;
    uint CascadeCount;
    int FilterSize;                        // 3, 5 or 7
    int DitheredBlend;
    int ShowCascades;
    int Enabled;
    float _pad0;
} uShadowParams;

// (Rid) Colors and material terms are mediump (RelaxedPrecision): mobile GPUs run fp16 math at up to twice the fp32 rate
// Positions, normals and anything with a large range (specular power, attenuation) stay highp
struct PBRParameters
{
    mediump vec3 Albedo;
    mediump float Metallic;
    mediump float Roughness;
    vec3 Normal;
    vec3 View;
};
PBRParameters gPBRParams;

float SampleShadowTap(vec2 baseUV, float u, float v, float depth)
{
    return texture(uShadowAtlas, vec3(baseUV + vec2(u, v) * uShadowParams.AtlasTexelSize, depth));
}

// Tent filtered PCF out of hardware 2x2 comparisons (The Witness, MJP's Shadows sample): the 3x3, 5x5 and 7x7 tents cost 4, 9 and 16 lookups
float SampleShadowPCF(vec2 atlasUV, float depth)
{
    vec2 uv = atlasUV / uShadowParams.AtlasTexelSize; // In texels
    vec2 baseUV = floor(uv + 0.5);
    float s = uv.x + 0.5 - baseUV.x;
    float t = uv.y + 0.5 - baseUV.y;
    baseUV = (baseUV - 0.5) * uShadowParams.AtlasTexelSize;

    float sum = 0.0;
    if (uShadowParams.FilterSize == 3)
    {
        float uw0 = 3.0 - 2.0 * s;
        float uw1 = 1.0 + 2.0 * s;
        float u0 = (2.0 - s) / uw0 - 1.0;
        float u1 = s / uw1 + 1.0;

        float vw0 = 3.0 - 2.0 * t;
        float vw1 = 1.0 + 2.0 * t;
        float v0 = (2.0 - t) / vw0 - 1.0;
        float v1 = t / vw1 + 1.0;

        sum += uw0 * vw0 * SampleShadowTap(baseUV, u0, v0, depth);
        sum += uw1 * vw0 * SampleShadowTap(baseUV, u1, v0, depth);
        sum += uw0 * vw1 * SampleShadowTap(baseUV, u0, v1, depth);
        sum += uw1 * vw1 * SampleShadowTap(baseUV, u1, v1, depth);
        return sum * (1.0 / 16.0);
    }
    else if (uShadowParams.FilterSize == 5)
    {
        float uw0 = 4.0 - 3.0 * s;
        float uw1 = 7.0;
        float uw2 = 1.0 + 3.0 * s;
        float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
        float u1 = (3.0 + s) / uw1;
        float u2 = s / uw2 + 2.0;

        float vw0 = 4.0 - 3.0 * t;
        float vw1 = 7.0;
        float vw2 = 1.0 + 3.0 * t;
        float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
        float v1 = (3.0 + t) / vw1;
        float v2 = t / vw2 + 2.0;

        sum += uw0 * vw0 * SampleShadowTap(baseUV, u0, v0, depth);
        sum += uw1 * vw0 * SampleShadowTap(baseUV, u1, v0, depth);
        sum += uw2 * vw0 * SampleShadowTap(baseUV, u2, v0, depth);
        sum += uw0 * vw1 * SampleShadowTap(baseUV, u0, v1, depth);
        sum += uw1 * vw1 * SampleShadowTap(baseUV, u1, v1, depth);
        sum += uw2 * vw1 * SampleShadowTap(baseUV, u2, v1, depth);
        sum += uw0 * vw2 * SampleShadowTap(baseUV, u0, v2, depth);
        sum += uw1 * vw2 * SampleShadowTap(baseUV, u1, v2, depth);
        sum += uw2 * vw2 * SampleShadowTap(baseUV, u2, v2, depth);
        return sum * (1.0 / 144.0);
    }

    float uw0 = 5.0 * s - 6.0;
    float uw1 = 11.0 * s - 28.0;
    float uw2 = -(11.0 * s + 17.0);
    float uw3 = -(5.0 * s + 1.0);
    float u0 = (4.0 * s - 5.0) / uw0 - 3.0;
    float u1 = (4.0 * s - 16.0) / uw1 - 1.0;
    float u2 = -(7.0 * s + 5.0) / uw2 + 1.0;
    float u3 = -s / uw3 + 3.0;

    float vw0 = 5.0 * t - 6.0;
    float vw1 = 11.0 * t - 28.0;
    float vw2 = -(11.0 * t + 17.0);
    float vw3 = -(5.0 * t + 1.0);
    float v0 = (4.0 * t - 5.0) / vw0 - 3.0;
    float v1 = (4.0 * t - 16.0) / vw1 - 1.0;
    float v2 = -(7.0 * t + 5.0) / vw2 + 1.0;
    float v3 = -t / vw3 + 3.0;

    sum += uw0 * vw0 * SampleShadowTap(baseUV, u0, v0, depth);
    sum += uw1 * vw0 * SampleShadowTap(baseUV, u1, v0, depth);
    sum += uw2 * vw0 * SampleShadowTap(baseUV, u2, v0, depth);
    sum += uw3 * vw0 * SampleShadowTap(baseUV, u3, v0, depth);
    sum += uw0 * vw1 * SampleShadowTap(baseUV, u0, v1, depth);
    sum += uw1 * vw1 * SampleShadowTap(baseUV, u1, v1, depth);
    sum += uw2 * vw1 * SampleShadowTap(baseUV, u2, v1, depth);
    sum += uw3 * vw1 * SampleShadowTap(baseUV, u3, v1, depth);
    sum += uw0 * vw2 * SampleShadowTap(baseUV, u0, v2, depth);
    sum += uw1 * vw2 * SampleShadowTap(baseUV, u1, v2, depth);
    sum += uw2 * vw2 * SampleShadowTap(baseUV, u2, v2, depth);
    sum += uw3 * vw2 * SampleShadowTap(baseUV, u3, v2, depth);
    sum += uw0 * vw3 * SampleShadowTap(baseUV, u0, v3, depth);
    sum += uw1 * vw3 * SampleShadowTap(baseUV, u1, v3, depth);
    sum += uw2 * vw3 * SampleShadowTap(baseUV, u2, v3, depth);
    sum += uw3 * vw3 * SampleShadowTap(baseUV, u3, v3, depth);
    return sum * (1.0 / 2704.0);
}

float SampleCascade(int cascade, vec3 N, float NdotL)
{
    vec4 scale = uShadowParams.CascadeScale[cascade];

    // Normal offset: the receiver is pushed off its surface by a few texels of this cascade, more at grazing angles
    float offset = uShadowParams.NormalBias * scale.w * 1.4142136 * (1.0 - NdotL);
    vec3 coord0 = vInput.ShadowCoord + mat3(uShadowParams.LightSpaceMatrix) * (N * offset);
    vec3 coord = coord0 * scale.xyz + uShadowParams.CascadeOffset[cascade].xyz;

    // This cascade's region of the atlas, kept 5 texels inside its edges so the widest filter never reads a neighbour
    float border = 5.0 * uShadowParams.AtlasTexelSize.y;
    vec2 uv = clamp(coord.xy * 0.5 + 0.5, vec2(border), vec2(1.0 - border));
    uv.x = (uv.x + float(cascade)) / float(uShadowParams.CascadeCount);
    return SampleShadowPCF(uv, min(coord.z, 1.0));
}

int gCascadeIndex = -1;

// Shadow term of the directional light ShadowPass rendered, L points toward the light
float CalculateDirectionalShadow(vec3 L)
{
    if (uShadowParams.Enabled == 0)
        return 1.0;

    float depth = vInput.ViewDepth;
    if (depth >= uShadowParams.Distance)
        return 1.0;

    int count = int(uShadowParams.CascadeCount);
    int cascade = count - 1;
    for (int i = 0; i < count - 1; i++)
    {
        if (depth < uShadowParams.CascadeEnds[i])
        {
            cascade = i;
            break;
        }
    }

    // Blend band at the end of every cascade but the last one (that one fades out instead)
    float blend = 0.0;
    if (cascade < count - 1)
    {
        float cascadeStart = cascade == 0 ? 0.0 : uShadowParams.CascadeEnds[cascade - 1];
        float cascadeEnd = uShadowParams.CascadeEnds[cascade];
        float bandStart = cascadeEnd - (cascadeEnd - cascadeStart) * uShadowParams.CascadeBlend;
        blend = clamp((depth - bandStart) / max(cascadeEnd - bandStart, 0.0001), 0.0, 1.0);
    }

    vec3 N = normalize(vInput.Normal); // Geometric normal, a normal map would make the offset noisy
    float NdotL = clamp(dot(N, L), 0.0, 1.0);

    float shadow;
    if (blend > 0.0 && uShadowParams.DitheredBlend == 1)
    {
        // One cascade per pixel, picked with interleaved gradient noise: a single lookup set, the filter hides the noise
        float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
        if (noise < blend)
            cascade++;
        shadow = SampleCascade(cascade, N, NdotL);
    }
    else
    {
        shadow = SampleCascade(cascade, N, NdotL);
        if (blend > 0.0)
            shadow = mix(shadow, SampleCascade(cascade + 1, N, NdotL), blend);
    }
    gCascadeIndex = cascade;

    float fade = clamp((depth - uShadowParams.FadeStart) / max(uShadowParams.Distance - uShadowParams.FadeStart, 0.0001), 0.0, 1.0);
    return mix(shadow, 1.0, fade);
}

vec3 CalculateNormal()
{
    if (uMaterial.UseNormalMap == 1)
    {
        vec3 normal = normalize(vInput.Normal);
        vec3 tangent = normalize(vInput.Tangent);
        vec3 bitangent = normalize(vInput.BiTangent);

        vec3 bumpMapNormal = texture(NormalMap, vInput.TexCoord).xyz;
        bumpMapNormal = 2.0 * bumpMapNormal - vec3(1.0);

        mat3 TBN = mat3(tangent, bitangent, normal);
        vec3 newNormal = TBN * bumpMapNormal;

        return normalize(newNormal);
    }

    return normalize(vInput.Normal);
}

vec3 CalculateMobilePBR(Light light, vec3 N, vec3 V, vec3 fragPos)
{   
    vec3 L;
    float attenuation = 1.0;

    if (light.PositionType.w == 0.0)
    {
        L = normalize(-light.PositionType.xyz);
    }
    else 
    {
        vec3 dir = light.PositionType.xyz - fragPos;
        float dist = length(dir);
        if (dist >= light.Radius)
            return vec3(0.0); // The window function below is 0 past the radius, skip the BRDF entirely
        L = normalize(dir);

        float distanceSquare = dist * dist;
        attenuation = 1.0 / max(distanceSquare, 0.0001);

        float factor = dist / light.Radius;
        float window = clamp(1.0 - pow(factor, 4.0), 0.0, 1.0);
        window *= window;

        attenuation *= window;
        attenuation = pow(attenuation, light.Falloff);
    }

    vec3 H = normalize(L + V);
    float dotNH = max(dot(N, H), 0.0);
    float dotNL = max(dot(N, L), 0.0);

    float alpha = clamp(gPBRParams.Roughness, 0.04, 1.0);
    float shininess = max(2.0 / (pow(alpha, 4.0)) - 2.0, 1.0);

    mediump vec3 f0 = vec3(0.04) * uMaterial.Reflectance;
    mediump vec3 specColor = mix(f0, gPBRParams.Albedo, gPBRParams.Metallic);
    mediump vec3 diffuseColor = gPBRParams.Albedo * (1.0 - gPBRParams.Metallic);

    float specNormalization = (shininess + 8.0) / (8.0 * 3.14159);
    float specularTerm = pow(dotNH, shininess) * specNormalization;

    mediump vec3 diffuse = (diffuseColor / 3.14159) * dotNL;
    vec3 specular = specColor * specularTerm * dotNL;

    vec3 lightIntensity = light.Color.rgb * light.Intensity;

    return (diffuse + specular) * lightIntensity * attenuation;
}

// Hemispherical Ambient GI with Vertex AO Modulation
vec3 CalculateFastGI(vec3 N, vec3 V)
{
    // Hemispherical diffuse ambient
    mediump float skyGroundWeight = N.y * 0.5 + 0.5;
    mediump vec3  rawSkyColor = mix(uLights.HorizonAmbient, uLights.SkyAmbient, max(N.y, 0.0));
    mediump vec3  totalAmbientDiffuse = mix(uLights.GroundAmbient, rawSkyColor, skyGroundWeight);

    // Metals have no diffuse
    mediump vec3 diffuseAmbient = totalAmbientDiffuse * gPBRParams.Albedo * (1.0 - gPBRParams.Metallic);

    // Environment reflection direction
    vec3  R = reflect(-V, N);
    mediump float reflSkyWeight = R.y * 0.5 + 0.5;
    mediump vec3  rawReflColor = mix(uLights.HorizonAmbient, uLights.SkyAmbient, max(R.y, 0.0));
    mediump vec3  environmentRefl = mix(uLights.GroundAmbient, rawReflColor, reflSkyWeight);

    mediump vec3 f0 = vec3(0.04) * uMaterial.Reflectance;
    mediump vec3 specColor = mix(f0, gPBRParams.Albedo, gPBRParams.Metallic);

    mediump float smoothness = 1.0 - gPBRParams.Roughness;
    mediump vec3  ambientEnv = mix(totalAmbientDiffuse, environmentRefl, smoothness * smoothness);
    mediump vec3  specularAmbient = ambientEnv * specColor;

    return (diffuseAmbient + specularAmbient);
}

vec3 VisualizeCascades(vec3 finalColor, int cascadeIndex)
{
    if (uShadowParams.ShowCascades == 1 && cascadeIndex != -1)
    {
        switch (cascadeIndex)
        {
            case 0: return finalColor *= vec3(1.0,  0.25, 0.25);
            case 1: return finalColor *= vec3(0.25, 1.0,  0.25);
            case 2: return finalColor *= vec3(1.0,  1.0,  0.25);
            case 3: return finalColor *= vec3(0.25, 0.25, 1.0 );
        }
    }
    return finalColor;
}

void main()
{
    if (uMaterial.UseAlbedoMap == 1)
    {
        mediump vec4 tex = texture(AlbedoMap, vInput.TexCoord);
        gPBRParams.Albedo = tex.rgb * uMaterial.Albedo;
    }
    else
    {
        gPBRParams.Albedo = uMaterial.Albedo;
    }

    gPBRParams.Normal = CalculateNormal();
    gPBRParams.Metallic = (uMaterial.UseMetallicMap == 1) 
        ? texture(RoughnessMetallicMap, vInput.TexCoord).b * uMaterial.Metallic 
        : uMaterial.Metallic;
    gPBRParams.Roughness = (uMaterial.UseRoughnessMap == 1) 
        ? texture(RoughnessMetallicMap, vInput.TexCoord).g * uMaterial.Roughness 
        : uMaterial.Roughness;

    gPBRParams.View = normalize(uFrame.CameraPos - vInput.WorldPos);

    // GI modulated by baked vertex AO
    vec3 ambient = CalculateFastGI(gPBRParams.Normal, gPBRParams.View);

    // Direct Lighting Accumulation
    vec3 directAccumulation = vec3(0.0);
    float shadow = -1.0; // Computed once, by the first directional light that actually lights this pixel

    for (uint i = 0; i < uMesh.LightCount; i++)
    {
        vec3 contribution = CalculateMobilePBR(uLights.Lights[i], gPBRParams.Normal, gPBRParams.View, vInput.WorldPos);

        if (uLights.Lights[i].PositionType.w == 0.0 && any(greaterThan(contribution, vec3(0.0)))) // Lit by a directional light
        {
            if (shadow < 0.0)
                shadow = CalculateDirectionalShadow(normalize(-uLights.Lights[i].PositionType.xyz));
            contribution *= shadow;
        }

        directAccumulation += contribution;
    }

    vec3 emission = uMaterial.Emission * uMaterial.EmissionIntensity;
    if (uMaterial.UseEmissionMap == 1)
        emission *= texture(EmissionMap, vInput.TexCoord).rgb;
    vec3 HDRColor = ambient + directAccumulation + emission;
    FinalColor = vec4(VisualizeCascades(HDRColor, gCascadeIndex), 1.0);
}

//SURGE:[Shader: Vertex]
#version 450
const int MAX_CASCADE_COUNT = 3;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBiTangent;
layout(location = 4) in vec2 aTexCoord;

layout(set = 0, binding = 0) uniform FrameUBO
{
    mat4 View;
    mat4 ViewProjection;
    mat4 InverseViewProjection;
    vec3 CameraPos;
    float _pad;
} uFrame;

// Per-instance transforms written by GeometryPass in draw order, gl_InstanceIndex includes firstInstance in Vulkan
layout(std430, set = 0, binding = 2) readonly buffer InstanceData
{
    mat4 Transforms[];
} uInstances;

struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 BiTangent;
    vec3 WorldPos;
    float ViewDepth;  // Positive view space depth, picks the shadow cascade
    vec3 ShadowCoord; // Cascade 0 light space position, the other cascades are a scale + offset of it
};
layout(location = 0) out VertexOutput vOutput;

layout(std140, set = 2, binding = 1) uniform ShadowUBO
{
    mat4 LightSpaceMatrix;                 // World -> cascade 0
    vec4 CascadeScale[MAX_CASCADE_COUNT];  // xyz: cascade 0 -> cascade i, w: world size of one shadow texel
    vec4 CascadeOffset[MAX_CASCADE_COUNT];
    vec4 CascadeEnds;                      // Positive view depth
    vec2 AtlasTexelSize;
    float NormalBias;                      // In shadow texels
    float CascadeBlend;
    float FadeStart;
    float Distance;
    uint CascadeCount;
    int FilterSize;                        // 3, 5 or 7
    int DitheredBlend;
    int ShowCascades;
    int Enabled;
    float _pad0;
} uShadowParams;

void main()
{
    mat4 model = uInstances.Transforms[gl_InstanceIndex];
    vec4 worldPos = model * vec4(aPosition, 1.0);

    vOutput.TexCoord = aTexCoord;
    vOutput.WorldPos = worldPos.xyz;
    vOutput.Tangent = mat3(model) * normalize(aTangent);
    vOutput.BiTangent = mat3(model) * normalize(aBiTangent);
    vOutput.Normal = mat3(model) * normalize(aNormal);

    vOutput.ViewDepth = -(uFrame.View * worldPos).z;
    vOutput.ShadowCoord = (uShadowParams.LightSpaceMatrix * worldPos).xyz; // Orthographic, w == 1

    gl_Position = uFrame.ViewProjection * worldPos;
}