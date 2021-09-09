// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/ReflectionData.hpp"
#include <shaderc/shaderc.h>

namespace Surge::VulkanUtils
{
    ShaderType ShaderTypeFromString(const String& type);
    String ShaderTypeToString(const ShaderType& type);
    shaderc_shader_kind ShadercShaderKindFromSurgeShaderType(const ShaderType& type);

    VkPrimitiveTopology GetVulkanPrimitiveTopology(PrimitiveTopology primitive);
    VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type);
    Vector<VkDescriptorSetLayout> GetDescriptorSetLayoutVectorFromHashMap(const HashMap<Uint, VkDescriptorSetLayout>& descriptorSetLayouts);
    Vector<VkPushConstantRange> GetPushConstantRangesVectorFromHashMap(const HashMap<String, VkPushConstantRange>& pushConstants);
    VkDescriptorType ShaderBufferTypeToVulkan(ShaderBuffer::Usage type);
    VkDescriptorType ShaderImageTypeToVulkan(ShaderResource::Usage type);
    VkShaderStageFlags GetShaderStagesFlagsFromShaderTypes(const Vector<ShaderType>& shaderStages);
    void CreateWindowSurface(VkInstance instance, Window* windowHandle, VkSurfaceKHR* surface);
}
