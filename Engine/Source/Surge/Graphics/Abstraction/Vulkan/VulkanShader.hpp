// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const Path& path);
        virtual ~VulkanShader() override;

        virtual void Reload() override;
        virtual const std::unordered_map<ShaderType, ShaderReflectionData>& GetReflectionData() const override { return mReflectionData; }
        HashMap<ShaderType, VkShaderModule> GetVulkanShaderModules() const { return mVkShaderModules; }
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const override { return mShaderSPIRVs; }
        virtual const Path& GetPath() const override { return mPath; }

        static VkShaderStageFlagBits GetVulkanShaderStage(ShaderType type);
    private:
        void ParseShader();
        void Compile();
        void Clear();
    private:
        Path mPath;
        std::unordered_map<ShaderType, String> mShaderSources{};
        std::unordered_map<ShaderType, VkShaderModule> mVkShaderModules{};
        std::unordered_map<ShaderType, ShaderReflectionData> mReflectionData{};
        Vector<SPIRVHandle> mShaderSPIRVs{};
    };
}
