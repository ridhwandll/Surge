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
        virtual const HashMap<ShaderType, ShaderReflectionData>& GetReflectionData() const override { return mReflectionData; }
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const override { return mShaderSPIRVs; }
        virtual const Path& GetPath() const override { return mPath; }

        const HashMap<ShaderType, VkShaderModule>& GetVulkanShaderModules() const { return mVkShaderModules; }

        static VkShaderStageFlagBits GetVulkanShaderStage(ShaderType type);
    private:
        void ParseShader();
        void Compile();
        void Clear();
    private:
        Path mPath;
        HashMap<ShaderType, String> mShaderSources{};
        HashMap<ShaderType, VkShaderModule> mVkShaderModules{};
        HashMap<ShaderType, ShaderReflectionData> mReflectionData{};
        Vector<SPIRVHandle> mShaderSPIRVs{};
    };
}
