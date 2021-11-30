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
    vec3 Tangent;
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
    vOutput.Tangent = aTangent;
    vOutput.BiTangent = aBiTangent;
    vOutput.Normal = normalize(aNormal) * mat3(uFrameData.Transform);
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
    vec3 Tangent;
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

    float Falloff;
    int _Padding_;
    int _Padding_1;
    int _Padding_2;
};
layout(binding = 0, set = 1) uniform Lights
{
    vec3 CameraPosition;
    int PointLightCount;

    PointLight PointLights[100];

} uLights;

// ---------- Material ----------
layout(set = 0, binding = 0) uniform Material
{
    vec3 Albedo;
    float Metalness;

    float Roughness;
    int UseNormalMap;

    int _Padding_;
    int _Padding_1;

} uMaterial;
layout(set = 2, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 2, binding = 2) uniform sampler2D NormalMap;
layout(set = 2, binding = 3) uniform sampler2D RoughnessMap;
layout(set = 2, binding = 4) uniform sampler2D MetalnessMap;

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
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
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

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
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

		vec3 F = fresnelSchlickRoughness(F0, max(0.0, dot(Lh, gPBRParams.View)), gPBRParams.Roughness);
		float D = NdfGGX(cosLh, gPBRParams.Roughness);
		float G = gaSchlickGGX(cosLi, gPBRParams.NdotV, gPBRParams.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - gPBRParams.Metalness);
		vec3 diffuseBRDF = kd * gPBRParams.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * gPBRParams.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0), vec3(10.0));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
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

    // Direct lighting calculation for analytical lights.
    vec3 directLighting = vec3(0.0);
    directLighting += CalculatePointLights(F0);

    // TODO: GI
    vec3 ambient = vec3(0.0);

    FinalColor = vec4(ambient, 1.0) + vec4(directLighting, 1.0);
    FinalColor = FinalColor / (FinalColor + vec4(1.0));
    FinalColor = pow(FinalColor, vec4(1.0/2.2));
}