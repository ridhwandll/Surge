// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Vertex]
#version 460 core
const int MAX_CASCADE_COUNT = 4;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBiTangent;
layout(location = 4) in vec2 aTexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 Transform;
    mat4 ViewProjectionMatrix;

} uMesh;

// Set 0 belongs to the renderer
layout(set = 0, binding = 0) uniform Camera
{
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;

} uFrameData;

struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 BiTangent;
    vec3 WorldPos;

    vec3 ViewSpacePos;
    vec4 LightSpaceVector[MAX_CASCADE_COUNT];
};
layout(location = 0) out VertexOutput vOutput;

layout(set = 3, binding = 0) uniform ShadowParams
{
    vec4 CascadeEnds;
    mat4 LightSpaceMatrix[4];
    int ShowCascades;
    uint CascadeCount;

    int ShadowQuality;
    int _Padding_;

}uShadowParams;

void main()
{
    vOutput.TexCoord = aTexCoord;
    vOutput.Tangent   = mat3(uMesh.Transform) * normalize(aTangent);
    vOutput.BiTangent = mat3(uMesh.Transform) * normalize(aBiTangent);
    vOutput.Normal    = mat3(uMesh.Transform) * normalize(aNormal);
    
    vOutput.WorldPos = vec3(uMesh.Transform * vec4(aPosition, 1.0));
    vOutput.ViewSpacePos = vec3(uFrameData.ViewMatrix * vec4(vOutput.WorldPos, 1.0));

    for(uint i = 0; i < uShadowParams.CascadeCount; i++)
        vOutput.LightSpaceVector[i] = uShadowParams.LightSpaceMatrix[i] * vec4(vOutput.WorldPos, 1.0);

    gl_Position = uMesh.ViewProjectionMatrix * vec4(vOutput.WorldPos, 1.0);
}

[SurgeShader: Pixel]
#version 460 core

const int MAX_CASCADE_COUNT = 4;

// ---------- Stage Inputs ----------
struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 BiTangent;
    vec3 WorldPos;

    vec3 ViewSpacePos;
    vec4 LightSpaceVector[MAX_CASCADE_COUNT];
};
layout(location = 0) in VertexOutput vInput;

// ---------- Stage Outputs ----------
layout(location = 0) out vec4 FinalColor;

// ---------- Constants ----------
const float PI = 3.141592;
const float Epsilon = 0.00001;
const vec3 Fdielectric = vec3(0.04); // Constant normal incidence Fresnel factor for all dielectrics.

layout(set = 5, binding = 0) readonly buffer VisibleLightIndicesBuffer
{
	int Indices[];

} sVisibleLightIndicesBuffer;

// ---------- Lights ----------
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

// Material - Set 1 and 2
layout(set = 1, binding = 0) uniform Material
{
    vec3 Albedo;
    float Metalness;

    float Roughness;
    int UseNormalMap;

    int _Padding_;
    int _Padding_1;

} uMaterial;
layout(set = 2, binding = 0) uniform sampler2D AlbedoMap;
layout(set = 2, binding = 1) uniform sampler2D NormalMap;
layout(set = 2, binding = 2) uniform sampler2D RoughnessMap;
layout(set = 2, binding = 3) uniform sampler2D MetalnessMap;

// Shadows - Set 3
layout(set = 3, binding = 0) uniform ShadowParams
{
    vec4 CascadeEnds;
    mat4 LightSpaceMatrix[4];
    int ShowCascades;
    uint CascadeCount;

    int ShadowQuality;
    int _Padding_;

}uShadowParams;
layout(set = 3, binding = 1) uniform sampler2D ShadowMap1;
layout(set = 3, binding = 2) uniform sampler2D ShadowMap2;
layout(set = 3, binding = 3) uniform sampler2D ShadowMap3;
layout(set = 3, binding = 4) uniform sampler2D ShadowMap4;

// Move to a Storage buffer later
layout(set = 4, binding = 0) uniform Lights
{
    vec3 CameraPosition;
    int PointLightCount;

    PointLight PointLights[100];
    DirectionalLight DirLight;

} uLights;

struct PBRParameters
{
    vec3 Albedo;
    float Metalness;
    float Roughness;

    vec3 Normal;
    vec3 View;
    float NdotV;
};
PBRParameters gPBRParams;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float NdfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float GaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float GaSchlickGGX(float cosLi, float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return GaSchlickG1(cosLi, k) * GaSchlickG1(NdotV, k);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Shlick's approximation of the Fresnel factor.
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 CalculateNormal()
{
   vec3 newNormal;
   if (uMaterial.UseNormalMap == 1)
   {
        vec3 normal = normalize(vInput.Normal);
        vec3 tangent = normalize(vInput.Tangent);
        vec3 bitangent = normalize(vInput.BiTangent);

        vec3 bumpMapNormal = texture(NormalMap, vInput.TexCoord).xyz;
        bumpMapNormal = 2.0 * bumpMapNormal - vec3(1.0);
        
        mat3 TBN = mat3(tangent, bitangent, normal);
        newNormal = TBN * bumpMapNormal;
        newNormal = normalize(newNormal); 
   }
   else
   {
        newNormal = normalize(vInput.Normal);
   }
   return newNormal;
}

vec3 CalculatePointLights(vec3 F0)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < uLights.PointLightCount; i++)
    {
        PointLight light = uLights.PointLights[i];

        vec3 Li = normalize(light.Position - vInput.WorldPos);
        float lightDistance = length(light.Position - vInput.WorldPos);
        vec3 Lh = normalize(Li + gPBRParams.View);

        float attenuation = clamp(1.0 - (lightDistance * lightDistance) / (light.Radius * light.Radius), 0.0, 1.0);
        attenuation *= mix(attenuation, 1.0, light.Falloff);

        vec3 Lradiance = light.Color * light.Intensity * attenuation;

        // Calculate angles between surface normal and various light vectors.
        float cosLi = max(0.0, dot(gPBRParams.Normal, Li));
        float cosLh = max(0.0, dot(gPBRParams.Normal, Lh));

        vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, gPBRParams.View)), gPBRParams.Roughness);
        float D = NdfGGX(cosLh, gPBRParams.Roughness);
        float G = GaSchlickGGX(cosLi, gPBRParams.NdotV, gPBRParams.Roughness);

        vec3 kd = (1.0 - F) * (1.0 - gPBRParams.Metalness);
        vec3 diffuseBRDF = kd * gPBRParams.Albedo;

        // Cook-Torrance
        vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * gPBRParams.NdotV);
        specularBRDF = clamp(specularBRDF, vec3(0.0), vec3(10.0));
        result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
    }
    return result;
}

float SampleShadowMap(int cascadeIndex, vec2 texCoords)
{
   if (cascadeIndex == 0)
       return texture(ShadowMap1, texCoords).r;
   if (cascadeIndex == 1)
       return texture(ShadowMap2, texCoords).r;
   if (cascadeIndex == 2)
       return texture(ShadowMap3, texCoords).r;
   if (cascadeIndex == 3)
       return texture(ShadowMap4, texCoords).r;
}
//Employ stochastic sampling
const int PCF_SAMPLES = 64;
const int PCSS_SAMPLES = 16;

const vec2 gPoissonDisk[64] = vec2[](
	vec2(-0.884081, 0.124488),
	vec2(-0.714377, 0.027940),
	vec2(-0.747945, 0.227922),
	vec2(-0.939609, 0.243634),
	vec2(-0.985465, 0.045534),
	vec2(-0.861367, -0.136222),
	vec2(-0.881934, 0.396908),
	vec2(-0.466938, 0.014526),
	vec2(-0.558207, 0.212662),
	vec2(-0.578447, -0.095822),
	vec2(-0.740266, -0.095631),
	vec2(-0.751681, 0.472604),
	vec2(-0.553147, -0.243177),
	vec2(-0.674762, -0.330730),
	vec2(-0.402765, -0.122087),
	vec2(-0.319776, -0.312166),
	vec2(-0.413923, -0.439757),
	vec2(-0.979153, -0.201245),
	vec2(-0.865579, -0.288695),
	vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),
	vec2(-0.604452, -0.544251),
	vec2(-0.418056, -0.587679),
	vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),
	vec2(-0.267004, -0.459702),
	vec2(-0.100006, -0.229116),
	vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),
	vec2(-0.763488, -0.543386),
	vec2(-0.549030, -0.750749),
	vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),
	vec2(-0.429392, -0.894892),
	vec2(-0.131597, 0.065058),
	vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),
	vec2(-0.294586, -0.891515),
	vec2(-0.629418, 0.379387),
	vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),
	vec2(0.022018, -0.263793),
	vec2(0.003879, -0.136073),
	vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),
	vec2(0.114133, -0.070053),
	vec2(0.163314, -0.217231),
	vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),
	vec2(0.035302, -0.619310),
	vec2(0.195646, -0.459022),
	vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),
	vec2(-0.628418, 0.507978),
	vec2(-0.508473, 0.458753),
	vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),
	vec2(-0.043643, 0.312119),
	vec2(0.132993, 0.085170),
	vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),
	vec2(0.265220, -0.596716),
	vec2(-0.009628, -0.483058),
	vec2(-0.018516, 0.435703)
);
vec2 SamplePoissonDisk(uint index)
{
  return gPoissonDisk[index % 64];
}

float GetShadowBias()
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(gPBRParams.Normal, uLights.DirLight.Direction)), MINIMUM_SHADOW_BIAS);
	return bias;
}

float MediumShadowsDirectionalLight(int cascade, vec3 shadowCoords)
{
	float bias = GetShadowBias();
    float sum = 0.0;
    vec2 texelSize = 1.0 / textureSize(ShadowMap1, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
	        float z = SampleShadowMap(cascade, (shadowCoords.xy * 0.5 + 0.5) + vec2(x, y) * texelSize).x;
            sum += step(shadowCoords.z, z + bias);
        }    
    }
    sum /= 9.0;
    return sum;
}

float HardShadowsDirectionalLight(int cascade, vec3 shadowCoords)
{
	float bias = GetShadowBias();
	float shadowMapDepth = SampleShadowMap(cascade, shadowCoords.xy * 0.5 + 0.5).x;
	return step(shadowCoords.z, shadowMapDepth + bias);
}

// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float receiverDistance)
{
	const float NEAR = 0.1;
	return uLights.DirLight.Size * (receiverDistance - NEAR) / uLights.CameraPosition.z;
}

float SearchRegionRadiusUV(float zWorld)
{
	const float lightzNear = 0.0;
	const float lightRadiusUV = 0.05;
	return lightRadiusUV * (zWorld - lightzNear) / zWorld;
}

float FindBlockerDistanceDirectionalLight(int cascade, vec3 shadowCoords)
{
	float bias = GetShadowBias();

	int blockers = 0;
	float avgBlockerDistance = 0.0;
	float searchWidth = SearchRegionRadiusUV(shadowCoords.z);
	for (int i = 0; i < PCSS_SAMPLES; i++)
	{
        vec2 disk = SamplePoissonDisk(i);
		float z = SampleShadowMap(cascade, (shadowCoords.xy * 0.5 + 0.5) + disk * searchWidth).x;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers);

	return -1.0;
}

float PCFDirectionalLight(int cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();

	float sum = 0.0;
	for (int i = 0; i < PCF_SAMPLES; i++)
	{
		vec2 offset = SamplePoissonDisk(i) * uvRadius;
		float z = SampleShadowMap(cascade, (shadowCoords.xy * 0.5 + 0.5) + offset).x;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / float(PCF_SAMPLES);
}

float PCSSDirectionalLight(int cascade, vec3 shadowCoords)
{
	float blockerDistance = FindBlockerDistanceDirectionalLight(cascade, shadowCoords);
	if (blockerDistance == -1.0) // No occlusion
		return 1.0;

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	float NEAR = 0.01;
	float uvRadius = penumbraWidth * uLights.DirLight.Size * NEAR / shadowCoords.z;
	uvRadius = min(uvRadius, 0.002);
	return PCFDirectionalLight(cascade, shadowCoords, uvRadius);
}

float CalculateShadows(int cascadeIndex, vec4 lightSpaceVector, vec3 normal, vec3 direction)
{
    vec3 projCoords = lightSpaceVector.xyz / lightSpaceVector.w; // Perspective divide
    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 0.0;

    float shadow = 0.0;

    if (uShadowParams.ShadowQuality == 0)
        shadow = HardShadowsDirectionalLight(cascadeIndex, projCoords); // Hard Shadows
    else if(uShadowParams.ShadowQuality == 1)
        shadow = MediumShadowsDirectionalLight(cascadeIndex, projCoords);
    else if(uShadowParams.ShadowQuality == 2)
        shadow = PCSSDirectionalLight(cascadeIndex, projCoords);

    return shadow;
}

vec3 CalculateDirectionaLight(vec3 F0, out int cascadeIdx)
{
    vec3 result = vec3(0.0);
    DirectionalLight light = uLights.DirLight;

    vec3 Li = light.Direction;
    vec3 Lradiance = light.Color * light.Intensity;
    vec3 Lh = normalize(Li + gPBRParams.View);

    // Calculate angles between surface normal and various light vectors.
    float cosLi = max(0.0, dot(gPBRParams.Normal, Li));
    float cosLh = max(0.0, dot(gPBRParams.Normal, Lh));

    vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, gPBRParams.View)), gPBRParams.Roughness);
    float D = NdfGGX(cosLh, gPBRParams.Roughness);
    float G = GaSchlickGGX(cosLi, gPBRParams.NdotV, gPBRParams.Roughness);

    vec3 kd = (1.0 - F) * (1.0 - gPBRParams.Metalness);
    vec3 diffuseBRDF = kd * gPBRParams.Albedo;

    // Cook-Torrance
    vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * gPBRParams.NdotV);
    specularBRDF = clamp(specularBRDF, vec3(0.0), vec3(10.0));
    result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;

    float shadow = 1.0;
    for (int j = 0; j < uShadowParams.CascadeCount; j++)
    {
        if (vInput.ViewSpacePos.z > -uShadowParams.CascadeEnds[j])
        {
            cascadeIdx = j;
            shadow = CalculateShadows(cascadeIdx, vInput.LightSpaceVector[j], gPBRParams.Normal, uLights.DirLight.Direction);
            break;
        }
    }
    return result * shadow;
}

void main()
{
    gPBRParams.Normal = CalculateNormal();
    gPBRParams.Albedo = texture(AlbedoMap, vInput.TexCoord).rgb * uMaterial.Albedo;
    gPBRParams.Metalness = texture(MetalnessMap, vInput.TexCoord).r * uMaterial.Metalness;
    gPBRParams.Roughness = texture(RoughnessMap, vInput.TexCoord).r * uMaterial.Roughness;
    gPBRParams.View = normalize(uLights.CameraPosition - vInput.WorldPos);
    gPBRParams.NdotV = max(dot(gPBRParams.Normal, gPBRParams.View), 0.0);
       
    // Fresnel reflectance at normal incidence (for metals use albedo color)
    vec3 F0 = mix(Fdielectric, gPBRParams.Albedo, gPBRParams.Metalness);
    int cascadeIndex;

    // Direct lighting calculation for analytical lights
    vec3 directLighting = vec3(0.0);
    directLighting += CalculatePointLights(F0);
    directLighting += CalculateDirectionaLight(F0, cascadeIndex);

    // TODO: GI
    vec3 ambient = vec3(0.0);

    FinalColor = vec4(ambient, 1.0) + vec4(directLighting, 1.0);
    FinalColor = FinalColor / (FinalColor + vec4(1.0));
    FinalColor = pow(FinalColor, vec4(1.0/2.2));


    if (uShadowParams.ShowCascades == 1)
    {
        switch (cascadeIndex)
        {
            case 0:
                FinalColor.rgb *= vec3(1.0, 0.25, 0.25);
                break;
            case 1:
                FinalColor.rgb *= vec3(0.25, 1.0, 0.25);
                break;
            case 2:
                FinalColor.rgb *= vec3(0.25, 0.25, 1.0);
                break;
            case 3:
                FinalColor.rgb *= vec3(1.0, 1.0, 0.25);
                break;
        }
    }
}