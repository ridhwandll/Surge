// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/ShaderReflector.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <shaderc/shaderc.hpp>

namespace Surge
{
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
        Clear();
        ParseShader();
        Compile();
    }

    void VulkanShader::Compile()
    {
        Scope<RenderContext>& context = GetRenderContext();
        VkDevice device = static_cast<VulkanDevice*>(context->GetInteralDevice())->GetLogicaldevice();

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

        // NOTE(Rid - AC3R) If we enable optimization, it removes the names :kekCry:
        //options.SetOptimizationLevel(shaderc_optimization_level_performance);

        for (auto&& [stage, source] : mShaderSources)
        {
            SPIRVHandle spirvHandle;
            shaderc::CompilationResult result = compiler.CompileGlslToSpv(source, VulkanUtils::ShadercShaderKindFromSurgeShaderType(stage), mPath.c_str(), options);
            if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                Log<LogSeverity::Error>("{0} Shader compilation failure!", VulkanUtils::ShaderTypeToString(stage));
                Log<LogSeverity::Error>("{0} Error(s): \n{1}", result.GetNumErrors(), result.GetErrorMessage());
                SG_ASSERT_INTERNAL("Shader Compilation failure!");
            }
            else
            {
                spirvHandle.Type = stage;
                spirvHandle.SPIRV = Vector<Uint>(result.cbegin(), result.cend());

                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = spirvHandle.SPIRV.size() * sizeof(Uint);
                createInfo.pCode = spirvHandle.SPIRV.data();

                VK_CALL(vkCreateShaderModule(device, &createInfo, nullptr, &mVkShaderModules[stage]));
                mShaderSPIRVs.push_back(spirvHandle);
            }
        }

        ShaderReflector reflector;
        mReflectionData = reflector.Reflect(mShaderSPIRVs);
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

            SG_ASSERT((int)VulkanUtils::ShaderTypeFromString(type), "Invalid shader type!");
            size_t nextLinePos = source.find_first_not_of("\r\n", eol);

            pos = source.find(typeToken, nextLinePos);
            mShaderSources[VulkanUtils::ShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
        }
    }

    VkShaderStageFlagBits VulkanShader::GetVulkanShaderStage(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::VertexShader:  return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::PixelShader:   return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::ComputeShader: return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }
        return VkShaderStageFlagBits();
    }
}
