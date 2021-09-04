// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <fstream>
#include <shaderc/shaderc.hpp>

namespace Surge
{
    namespace Utils
    {
        ShaderType ShaderTypeFromString(const String& type)
        {
            if (type == "Vertex]")  return ShaderType::VertexShader;
            if (type == "Pixel]")   return ShaderType::PixelShader;
            if (type == "Compute]") return ShaderType::ComputeShader;

            return ShaderType::None;
        }

        String ShaderTypeToString(const ShaderType& type)
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

        shaderc_shader_kind ShadercShaderKindFromSurgeShaderType(const ShaderType& type)
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

    VulkanShader::VulkanShader(const Path& path)
        : mPath(path)
    {
        Reload();
    }

    VulkanShader::~VulkanShader()
    {
        Clear();
    }

    void VulkanShader::Reload()
    {
        Timer t;
        Clear();
        ParseShader();
        Compile();
        Log("{0} - Shader compilation took: {1}", mPath, t.Elapsed());
    }

    void VulkanShader::Compile()
    {
        Scope<RenderContext>& context = GetRenderContext();
        VkDevice device = static_cast<VulkanDevice*>(context->GetInteralDevice())->GetLogicaldevice();

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        for (auto&& [stage, source] : mShaderSources)
        {
            Vector<Uint>& spirv = mShaderSPIRVs[stage];
            shaderc::CompilationResult result = compiler.CompileGlslToSpv(source, Utils::ShadercShaderKindFromSurgeShaderType(stage), mPath.c_str(), options);
            if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                Log<LogSeverity::Error>("{0} Shader compilation failure!", Utils::ShaderTypeToString(stage));
                Log<LogSeverity::Error>("{0} Error(s): \n{1}", result.GetNumErrors(), result.GetErrorMessage());
                SG_ASSERT_INTERNAL("Shader Compilation failure!");
            }
            else
            {
                spirv = Vector<Uint>(result.cbegin(), result.cend());

                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = spirv.size() * 4;
                createInfo.pCode = spirv.data();

                VK_CALL(vkCreateShaderModule(device, &createInfo, nullptr, &mVkShaderModules[stage]));
            }
        }
    }

    void VulkanShader::Clear()
    {
        VkDevice device = static_cast<VulkanDevice*>(GetRenderContext()->GetInteralDevice())->GetLogicaldevice();

        mShaderSources.clear();
        mShaderSPIRVs.clear();

        for (auto&& [stage, source] : mVkShaderModules)
            vkDestroyShaderModule(device, mVkShaderModules[stage], nullptr);

        mVkShaderModules.clear();
    }

    void VulkanShader::ParseShader()
    {
        String source = Filesystem::ReadFile(mPath);

        const char* typeToken = "[SurgeShader:";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            size_t begin = pos + typeTokenLength + 1;
            String type = source.substr(begin, eol - begin);

            SG_ASSERT((int)Utils::ShaderTypeFromString(type), "Invalid shader type!");
            size_t nextLinePos = source.find_first_not_of("\r\n", eol);

            pos = source.find(typeToken, nextLinePos);
            mShaderSources[Utils::ShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
        }
    }
}
