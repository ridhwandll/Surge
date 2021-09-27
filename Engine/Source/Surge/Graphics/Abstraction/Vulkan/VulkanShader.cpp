// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <json/json.hpp>
// TODO: Temporary, we don't have an asset manager yet
#define TEMP_ASSET_PATH "Engine/Assets/Temp"
#define SHADER_CACHE_PATH "Engine/Assets/Temp/ShaderCache"
#define SHADER_HASH_CACHE_PATH "Engine/Assets/Temp/ShaderCache/ShaderHash.txt"

#ifdef SURGE_DEBUG
#define SHADER_LOG(...) Log<Severity::Debug>(__VA_ARGS__);
#else
#define SHADER_LOG(...)
#endif

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
        SCOPED_TIMER("Shader({0}) Compilation", Filesystem::GetNameWithExtension(mPath));
        Clear();
        ParseShader();
        Compile();
        CreateVulkanDescriptorSetLayouts();
        CreateVulkanPushConstantRanges();
    }

    void VulkanShader::Compile()
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        bool saveHash = false;

        // NOTE(Rid - AC3R) If we enable optimization, it removes the names :kekCry:
        //options.SetOptimizationLevel(shaderc_optimization_level_performance);
        for (auto&& [stage, source] : mShaderSources)
        {
            SPIRVHandle spirvHandle;
            spirvHandle.Type = stage;

            String path = GetCachePath(stage);

            bool compile = false;
            const bool existsInCache = Filesystem::Exists(path);
            const HashCode cacheHashCode = GetCacheHashCode(stage);
            const HashCode currentHashCode = mHashCodes.at(stage);

            if (!existsInCache || (existsInCache && cacheHashCode != currentHashCode))
            {
                compile = true;
                saveHash = true;
            }

            // Load or create the SPIRV
            if (!compile)
            {
                // Load from cache
                FILE* f;
                fopen_s(&f, path.c_str(), "rb");
                if (f)
                {
                    fseek(f, 0, SEEK_END);
                    uint64_t size = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    spirvHandle.SPIRV = Vector<Uint>(size / sizeof(Uint));
                    fread(spirvHandle.SPIRV.data(), sizeof(Uint), spirvHandle.SPIRV.size(), f);
                    fclose(f);
                    SHADER_LOG("Loaded Shader from cache: {0}", path);
                }
            }
            else
            {
                // Compile, not present in cache
                shaderc::CompilationResult result = compiler.CompileGlslToSpv(source, VulkanUtils::ShadercShaderKindFromSurgeShaderType(stage), mPath.c_str(), options);
                if (result.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    Log<Severity::Error>("{0} Shader compilation failure!", VulkanUtils::ShaderTypeToString(stage));
                    Log<Severity::Error>("{0} Error(s): \n{1}", result.GetNumErrors(), result.GetErrorMessage());
                    SG_ASSERT_INTERNAL("Shader Compilation failure!");
                }
                else
                {
                    spirvHandle.SPIRV = Vector<Uint>(result.cbegin(), result.cend());
                    WriteSPIRVToFile(spirvHandle); // Cache the shader
                }
            }
            SG_ASSERT(!spirvHandle.SPIRV.empty(), "Invalid SPIRV!");

            // Create the VkShaderModule
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = spirvHandle.SPIRV.size() * sizeof(Uint);
            createInfo.pCode = spirvHandle.SPIRV.data();
            VK_CALL(vkCreateShaderModule(device, &createInfo, nullptr, &mVkShaderModules[stage]));
            mShaderSPIRVs.push_back(spirvHandle);
        }
        ShaderReflector reflector;
        mReflectionData = reflector.Reflect(mShaderSPIRVs);
        if (saveHash)
            WriteShaderHashToFile();
    }

    void VulkanShader::Clear()
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        mShaderSources.clear();
        mShaderSPIRVs.clear();

        for (auto&& [stage, source] : mVkShaderModules)
            vkDestroyShaderModule(device, mVkShaderModules[stage], nullptr);

        for (auto& descriptorSetLayout : mDescriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout.second, nullptr);

        mDescriptorSetLayouts.clear();
        mPushConstants.clear();
        mVkShaderModules.clear();
    }

    void VulkanShader::CreateVulkanDescriptorSetLayouts()
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        // Iterate through all the sets and creating the layouts
        // (descriptor layouts use HashMap<Uint, VkDescriptorSetLayout> because the Uint specifies at which set number the layout is going to be used
        const Vector<Uint>& descriptorSetCount = mReflectionData.GetDescriptorSetCount();
        for (const Uint& descriptorSet : descriptorSetCount)
        {
            Vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (const ShaderBuffer& buffer : mReflectionData.GetBuffers())
            {
                if (buffer.Set != descriptorSet)
                    continue;

                VkDescriptorSetLayoutBinding& LayoutBinding = layoutBindings.emplace_back();
                LayoutBinding.binding = buffer.Binding;
                LayoutBinding.descriptorCount = 1; // TODO: Need to add arrays
                LayoutBinding.descriptorType = VulkanUtils::ShaderBufferTypeToVulkan(buffer.Type);
                LayoutBinding.stageFlags = VulkanUtils::GetShaderStagesFlagsFromShaderTypes(buffer.ShaderStages);
            }

            for (const ShaderResource& texture : mReflectionData.GetResources())
            {
                if (texture.Set != descriptorSet)
                    continue;

                VkDescriptorSetLayoutBinding& LayoutBinding = layoutBindings.emplace_back();
                LayoutBinding.binding = texture.Binding;
                LayoutBinding.descriptorCount = 1; // TODO: Need to add arrays
                LayoutBinding.descriptorType = VulkanUtils::ShaderImageTypeToVulkan(texture.Type);
                LayoutBinding.stageFlags = VulkanUtils::GetShaderStagesFlagsFromShaderTypes(texture.ShaderStages);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.flags = 0;
            layoutInfo.bindingCount = static_cast<Uint>(layoutBindings.size());
            layoutInfo.pBindings = layoutBindings.data();

            VK_CALL(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayouts[descriptorSet]));
        }
    }

    void VulkanShader::CreateVulkanPushConstantRanges()
    {
        for (const ShaderPushConstant& pushConstant : mReflectionData.GetPushConstantBuffers())
        {
            VkPushConstantRange& pushConstantRange = mPushConstants[pushConstant.BufferName];
            pushConstantRange.offset = 0;
            pushConstantRange.size = pushConstant.Size;
            pushConstantRange.stageFlags = VulkanUtils::GetShaderStagesFlagsFromShaderTypes(pushConstant.ShaderStages);
        }
    }

    void VulkanShader::WriteSPIRVToFile(const SPIRVHandle& spirvHandle)
    {
        std::filesystem::create_directory(TEMP_ASSET_PATH);
        std::filesystem::create_directory(SHADER_CACHE_PATH);

        String path = GetCachePath(spirvHandle.Type);
        FILE* f;
        fopen_s(&f, path.c_str(), "wb");
        if (f)
        {
            fwrite(spirvHandle.SPIRV.data(), sizeof(Uint), spirvHandle.SPIRV.size(), f);
            fclose(f);
            SHADER_LOG("Cached Shader at: {0}", path);
        }
    }

    void VulkanShader::WriteShaderHashToFile() const
    {
        FILE* f = nullptr;
        errno_t e = 0;

        e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "r");

        // If the file doesn't exists, create an empty file
        if (e == ENOENT)
        {
            e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "w");
            if (f)
                fclose(f);
        }

        // Load in the contents of the file, because we append to it later
        String previousContents;
        if (f)
        {
            fseek(f, 0, SEEK_END);
            uint64_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            previousContents.resize(size / sizeof(char));
            fread(previousContents.data(), sizeof(char), previousContents.size(), f);
            fclose(f);
            remove(SHADER_HASH_CACHE_PATH); // Remove the old file
        }

        // Create A JSON, from the previous file for writing the new contents
        nlohmann::json j = previousContents.empty() ? nlohmann::json() : nlohmann::json::parse(previousContents);

        // For each HashCodes, update it
        for (auto& e : mHashCodes)
        {
            String name = GetCacheName(e.first);
            j[name] = e.second;
        }

        String result = j.dump(4);
        e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

    String VulkanShader::GetCachePath(const ShaderType& type) const
    {
        String path = fmt::format("{0}/{1}", SHADER_CACHE_PATH, GetCacheName(type));
        return path;
    }

    String VulkanShader::GetCacheName(const ShaderType& type) const
    {
        String name = fmt::format("{0}.{1}.spv", Filesystem::GetNameWithExtension(mPath), ShaderTypeToString(type));
        return name;
    }

    HashCode VulkanShader::GetCacheHashCode(const ShaderType& type) const
    {
        FILE* f = nullptr;
        errno_t e = fopen_s(&f, SHADER_HASH_CACHE_PATH, "r");
        String previousContents;
        if (f)
        {
            fseek(f, 0, SEEK_END);
            uint64_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            previousContents.resize(size / sizeof(char));
            fread(previousContents.data(), sizeof(char), previousContents.size(), f);
            fclose(f);
        }

        String name = GetCacheName(type);
        nlohmann::json j = previousContents.empty() ? nlohmann::json() : nlohmann::json::parse(previousContents);

        HashCode result = 0;
        if (j.contains(name))
            result = j[name];
        return result;
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
            size_t nextLinePos = source.find_first_not_of("\r\n", eol);

            ShaderType shaderType = VulkanUtils::ShaderTypeFromString(type);
            SG_ASSERT((int)shaderType, "Invalid shader type!");
            pos = source.find(typeToken, nextLinePos);
            mShaderSources[shaderType] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
            mHashCodes[shaderType] = Hash().Generate<String>(mShaderSources.at(shaderType));
        }
    }
}
