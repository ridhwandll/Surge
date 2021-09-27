// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/ShaderReflector.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const Path& path);
        virtual ~VulkanShader() override;

        virtual void Reload() override;
        virtual const ShaderReflectionData& GetReflectionData() const override { return mReflectionData; }
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const override { return mShaderSPIRVs; }
        virtual const Path& GetPath() const override { return mPath; }
        virtual const HashCode& GetHash(const ShaderType& type) const override { return mHashCodes.at(type); }

        // Vulkan Specific (Used by different Pipelines)
        HashMap<ShaderType, VkShaderModule>& GetVulkanShaderModules() { return mVkShaderModules; }
        HashMap<Uint, VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return mDescriptorSetLayouts; }
        HashMap<String, VkPushConstantRange>& GetPushConstantRanges() { return mPushConstants; }
    private:
        void ParseShader();
        void Compile();
        void Clear();
        void CreateVulkanDescriptorSetLayouts();
        void CreateVulkanPushConstantRanges();
        void WriteSPIRVToFile(const SPIRVHandle& spirvHandle);
        void WriteShaderHashToFile() const;
        String GetCachePath(const ShaderType& type) const;
        String GetCacheName(const ShaderType& type) const;
        HashCode GetCacheHashCode(const ShaderType& type) const;
    private:
        Path mPath;
        HashMap<ShaderType, HashCode> mHashCodes;
        HashMap<ShaderType, String> mShaderSources;
        HashMap<ShaderType, VkShaderModule> mVkShaderModules;
        HashMap<Uint, VkDescriptorSetLayout> mDescriptorSetLayouts;
        HashMap<String, VkPushConstantRange> mPushConstants;

        Vector<SPIRVHandle> mShaderSPIRVs{};
        ShaderReflectionData mReflectionData;
    };
}
