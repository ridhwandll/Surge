// Copyright (c) - SurgeTechnologies - All rights reserved
[SurgeShader: Vertex]
#version 460 core

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
    vec4 LightSpaceVector[4];
};
layout(location = 0) out VertexOutput vOutput;

layout(set = 3, binding = 0) uniform ShadowParams
{
    vec4 CascadeEnds;
    mat4 LightSpaceMatrix[4];

}uShadowParams;

void main()
{
    vOutput.TexCoord = aTexCoord;
    vOutput.Tangent   = normalize(aTangent) * mat3(uMesh.Transform);
    vOutput.BiTangent = normalize(aBiTangent) * mat3(uMesh.Transform);
    vOutput.Normal    = normalize(aNormal) * mat3(uMesh.Transform);
    
    vOutput.WorldPos = vec3(uMesh.Transform * vec4(aPosition, 1.0));
    vOutput.ViewSpacePos = vec3(uFrameData.ViewMatrix * vec4(vOutput.WorldPos, 1.0));

    vOutput.LightSpaceVector[0] = uShadowParams.LightSpaceMatrix[0] * vec4(vOutput.WorldPos, 1.0);
    vOutput.LightSpaceVector[1] = uShadowParams.LightSpaceMatrix[1] * vec4(vOutput.WorldPos, 1.0);
    vOutput.LightSpaceVector[2] = uShadowParams.LightSpaceMatrix[2] * vec4(vOutput.WorldPos, 1.0);
    vOutput.LightSpaceVector[3] = uShadowParams.LightSpaceMatrix[3] * vec4(vOutput.WorldPos, 1.0);

    gl_Position = uMesh.ViewProjectionMatrix * vec4(vOutput.WorldPos, 1.0);
}

[SurgeShader: Pixel]
#version 460 core

const int NUM_CASCADES = 4;

// ---------- Stage Inputs ----------
struct VertexOutput
{
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 BiTangent;
    vec3 WorldPos;

    vec3 ViewSpacePos;
    vec4 LightSpaceVector[NUM_CASCADES];
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
struct DirectionalLight
{
    vec3 Direction;
    float Intensity;

    vec3 Color;
    float _Padding_;
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

    int _Padding_2;
    int _Padding_3;
    int _Padding_4;

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

float CalculateShadows(int cascadeIndex, vec4 lightSpaceVector)
{
    vec3 projCoords = lightSpaceVector.xyz / lightSpaceVector.w; // Perspective divide
    float currentDepth = projCoords.z;

    float closestDepth = SampleShadowMap(cascadeIndex, projCoords.xy * 0.5 + 0.5); 

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(ShadowMap1, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = SampleShadowMap(cascadeIndex, (projCoords.xy * 0.5 + 0.5) + vec2(x, y) * texelSize); 
            shadow += currentDepth > pcfDepth ? 0.0 : 1.0;        
        }    
    }
    shadow /= 9.0;

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
	float G = gaSchlickGGX(cosLi, gPBRParams.NdotV, gPBRParams.Roughness);

	vec3 kd = (1.0 - F) * (1.0 - gPBRParams.Metalness);
	vec3 diffuseBRDF = kd * gPBRParams.Albedo;

	// Cook-Torrance
	vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * gPBRParams.NdotV);
	specularBRDF = clamp(specularBRDF, vec3(0.0), vec3(10.0));
	result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;

    float shadow = 1.0;
    for (int j = 0; j < NUM_CASCADES; j++)
    {
        if (vInput.ViewSpacePos.z > -uShadowParams.CascadeEnds[j])
        {
            cascadeIdx = j;
            shadow = CalculateShadows(cascadeIdx, vInput.LightSpaceVector[j]);
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