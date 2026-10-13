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
    vec3 ViewSpacePos;
    vec4 LightSpaceVectors[MAX_CASCADE_COUNT];
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
    int UseEmissionMap; // Fills the std140 padding before Emission
    vec3 Emission;
    float EmissionIntensity;
} uMaterial;

layout(set = 1, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 1, binding = 2) uniform sampler2D NormalMap;
layout(set = 1, binding = 3) uniform sampler2D RoughnessMetallicMap;
layout(set = 1, binding = 4) uniform sampler2D EmissionMap;

// Set 2: Shadows
layout(set = 2, binding = 0) uniform sampler2DShadow uShadowMap0;
layout(set = 2, binding = 1) uniform sampler2DShadow uShadowMap1;
layout(set = 2, binding = 2) uniform sampler2DShadow uShadowMap2;
layout(set = 2, binding = 3) uniform ShadowUBO
{
    vec4 CascadeEnds;
    mat4 LightSpaceMatrix[MAX_CASCADE_COUNT];
    uint CascadeCount;
    int ShowCascades;
    float _pad0, pad1;
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

const vec2 poissonDisk[4] = vec2[](
    vec2(-0.94201624, -0.39906216), 
    vec2( 0.94558609, -0.76890725), 
    vec2(-0.09418410,  0.92938870), 
    vec2( 0.34495938,  0.29387760)
);

float SampleShadowMap(int cascadeIndex, vec3 texCoords)
{
    if (cascadeIndex == 0)
        return texture(uShadowMap0, texCoords).r;
    if (cascadeIndex == 1)
        return texture(uShadowMap1, texCoords).r;
    if (cascadeIndex == 2)
        return texture(uShadowMap2, texCoords).r;
    return 1.0;
}

float SampleShadow(int cascade, vec3 shadowCoord)
{
    vec2 uv = shadowCoord.xy * 0.5 + 0.5;
    float depth = shadowCoord.z;

    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap0, 0)); // All cascades share one resolution
    float shadow = 0.0;
    for (int i = 0; i < 4; i++)
        shadow += SampleShadowMap(cascade, vec3(uv + poissonDisk[i] * texelSize, depth));

    return shadow / 4.0;
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

vec3 VisuaLizeCascades(vec3 finalColor, int cascadeIndex)
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
    int cascadeIndex = -1;

    for (uint i = 0; i < uMesh.LightCount; i++)
    {
        vec3 contribution = CalculateMobilePBR(uLights.Lights[i], gPBRParams.Normal, gPBRParams.View, vInput.WorldPos);

        if (uLights.Lights[i].PositionType.w == 0.0) // Directional light
        {
            float shadow = 1.0;
            for (int j = 0; j < uShadowParams.CascadeCount; j++)
            {
                if (vInput.ViewSpacePos.z > -uShadowParams.CascadeEnds[j])
                {
                    cascadeIndex = j;
                    vec3 shadowCoords = vInput.LightSpaceVectors[j].xyz / vInput.LightSpaceVectors[j].w;
                    shadow = SampleShadow(cascadeIndex, shadowCoords);
                    break;
                }
            }
            contribution *= shadow;
        }

        directAccumulation += contribution;
    }

    vec3 emission = uMaterial.Emission * uMaterial.EmissionIntensity;
    if (uMaterial.UseEmissionMap == 1)
        emission *= texture(EmissionMap, vInput.TexCoord).rgb;
    vec3 HDRColor = ambient + directAccumulation + emission;
    FinalColor = vec4(VisuaLizeCascades(HDRColor, cascadeIndex), 1.0);
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
    vec3 ViewSpacePos;
    vec4 LightSpaceVectors[MAX_CASCADE_COUNT];
};
layout(location = 0) out VertexOutput vOutput;

layout(set = 2, binding = 3) uniform ShadowUBO
{
    vec4 CascadeEnds;
    mat4 LightSpaceMatrix[MAX_CASCADE_COUNT];
    uint CascadeCount;
    int ShowCascades;
    float _pad0, pad1;
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

    vOutput.ViewSpacePos = vec3(uFrame.View * worldPos);

    for(uint i = 0; i < MAX_CASCADE_COUNT; i++)
        vOutput.LightSpaceVectors[i] = uShadowParams.LightSpaceMatrix[i] * worldPos;

    gl_Position = uFrame.ViewProjection * worldPos;
}