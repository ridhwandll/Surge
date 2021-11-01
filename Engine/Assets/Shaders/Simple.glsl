// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Vertex]
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBiTangent;
layout(location = 4) in vec2 aTexCoord;

struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 BiTangent;
    vec3 WorldPos;
};
layout(location = 0) out VertexOutput vOutput;

layout(push_constant) uniform PushConstants
{
    mat4 ViewProjection;
    mat4 Transform;
} uFrameData;

void main()
{
    vOutput.TexCoord = aTexCoord;
    vOutput.Normal = normalize(aNormal);
    vOutput.WorldPos = vec3(uFrameData.Transform * vec4(aPosition, 1.0f));
    gl_Position = uFrameData.ViewProjection * vec4(vOutput.WorldPos, 1.0f);
}

[SurgeShader: Pixel]
#version 460 core

// ---------- Stage Inputs ----------
struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 BiTangent;
    vec3 WorldPos;
};
layout(location = 0) in VertexOutput vInput;

// ---------- Stage Outputs ----------
layout(location = 0) out vec4 FinalColor;

// ---------- Constants ----------
const float PI = 3.141592;
const float Epsilon = 0.00001;
const vec3 Fdielectric = vec3(0.04); // Constant normal incidence Fresnel factor for all dielectrics.

// ---------- Lights ----------
struct PointLight
{
    vec3 Position;
    float Intensity;
    vec3 Color;
    float Radius;
};
layout(binding = 0, set = 1) uniform Lights
{
    vec3 CameraPosition;
    uint PointLightCount;

    PointLight PointLights[100];

} uLights;

// ---------- Material ----------
layout(binding = 0, set = 0) uniform Material
{
    vec3 Albedo;
    float Metalness;

    float Roughness;
    float AO;
    vec2 _Padding_;
} uMaterial;

struct PBRParameters
{
    vec3 Albedo;
    float Metalness;
    float Roughness;
    float AO;
};

// GGX/Towbridge-Reitz normal distribution function, uses Disney's reparametrization of alpha = roughness^2
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSquare = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = alphaSquare;
    float denom = (NdotH2 * (alphaSquare - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Shlick's approximation of the Fresnel factor
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
vec3 CalculateLight(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 albedo, float roughness, float metalness)
{
    // Solving: DFG/4(ωo⋅n)(ωi⋅n)
    vec3 H = normalize(V + L);

    // Determine F
    vec3 F0 = mix(Fdielectric, albedo, metalness);
    float cosTheta = max(dot(H, V), 0.0);
    vec3 F = FresnelSchlick(F0, cosTheta);

    // Determine D and G
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = D * G * F; // (D*G*F)
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0); // 4(ωo⋅n)(ωi⋅n)
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;

    kD *= 1.0 - metalness;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    PBRParameters pbrParams;
    pbrParams.Albedo = uMaterial.Albedo;
    pbrParams.Metalness = uMaterial.Metalness;
    pbrParams.Roughness = uMaterial.Roughness;

    vec3 N = normalize(vInput.Normal); // Normal
    vec3 V = normalize(uLights.CameraPosition - vInput.WorldPos); // Outgoing light direction (vector from world-space fragment position to the "eye")
         
    // Fresnel reflectance at normal incidence (for metals use albedo color)
    vec3 F0 = mix(Fdielectric, pbrParams.Albedo, pbrParams.Metalness);

    // Direct lighting calculation for analytical lights.
    vec3 directLighting = vec3(0.0);
    for(int p = 0; p < uLights.PointLightCount; ++p)
    {
        vec3 dir = uLights.PointLights[p].Position - vInput.WorldPos;
        vec3 L = normalize(dir);
        float dist = length(dir);

        // Calculate attenuation and use it to get the radiance
        float attenuation = clamp(1.0 - (dist * dist) / (uLights.PointLights[p].Radius * uLights.PointLights[p].Radius), 0.0, 1.0);

        vec3 radiance = uLights.PointLights[p].Color * uLights.PointLights[p].Intensity * attenuation;
        directLighting += CalculateLight(N, L, V, max(radiance, vec3(0.0)), pbrParams.Albedo, pbrParams.Roughness, pbrParams.Metalness);
    }

    // TODO: GI
    vec3 ambient = vec3(0.0);

    FinalColor = vec4(ambient, 1.0) + vec4(directLighting, 1.0);
    FinalColor = FinalColor / (FinalColor + vec4(1.0));
    FinalColor = pow(FinalColor, vec4(1.0/2.2));
}