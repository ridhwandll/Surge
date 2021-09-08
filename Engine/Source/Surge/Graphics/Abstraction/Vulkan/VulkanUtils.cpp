// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"

namespace Surge
{
    ShaderType VulkanUtils::ShaderTypeFromString(const String& type)
    {
        if (type == "Vertex]")  return ShaderType::VertexShader;
        if (type == "Pixel]")   return ShaderType::PixelShader;
        if (type == "Compute]") return ShaderType::ComputeShader;

        return ShaderType::None;
    }

    String VulkanUtils::ShaderTypeToString(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::VertexShader:  return "Vertex";
        case ShaderType::PixelShader:   return "Pixel";
        case ShaderType::ComputeShader: return "Compute";
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }
        return "";
    }

    shaderc_shader_kind VulkanUtils::ShadercShaderKindFromSurgeShaderType(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::VertexShader:  return shaderc_glsl_vertex_shader;
        case ShaderType::PixelShader:   return shaderc_glsl_fragment_shader;
        case ShaderType::ComputeShader: return shaderc_glsl_compute_shader;
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }

        return static_cast<shaderc_shader_kind>(-1);
    }
}
