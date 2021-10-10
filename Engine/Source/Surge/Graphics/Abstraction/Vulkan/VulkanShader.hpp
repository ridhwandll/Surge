// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ShaderReflector.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const Path& path);
        virtual ~VulkanShader() override;

        virtual void Load(const HashMap<ShaderType, bool>& compileStages = {}) override;
        virtual void Reload() override;
        virtual void AddReloadCallback(const std::function<void()> callback) override { mCallbacks.push_back(callback); }
        virtual const ShaderReflectionData& GetReflectionData() const override { return mReflectionData; }
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const override { return mShaderSPIRVs; }
        virtual const Path& GetPath() const override { return mPath; }
        virtual const HashMap<ShaderType, String>& GetSources() const override { return mShaderSources; }
        virtual const HashCode& GetHash(const ShaderType& type) const override { return mHashCodes.at(type); }
        virtual const HashMap<ShaderType, HashCode>& GetHashCodes() const override { return mHashCodes; }

        // Vulkan Specific (Used by different Pipelines)
        HashMap<ShaderType, VkShaderModule>& GetVulkanShaderModules() { return mVkShaderModules; }
        HashMap<Uint, VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return mDescriptorSetLayouts; }
        HashMap<String, VkPushConstantRange>& GetPushConstantRanges() { return mPushConstants; }

    private:
        void ParseShader();
        void Compile(const HashMap<ShaderType, bool>& forceCompileStages);
        void Clear();
        void CreateVulkanDescriptorSetLayouts();
        void CreateVulkanPushConstantRanges();

    private:
        Path mPath;
        HashMap<ShaderType, HashCode> mHashCodes;
        HashMap<ShaderType, String> mShaderSources;
        HashMap<ShaderType, VkShaderModule> mVkShaderModules;
        HashMap<Uint, VkDescriptorSetLayout> mDescriptorSetLayouts;
        HashMap<String, VkPushConstantRange> mPushConstants;

        Vector<SPIRVHandle> mShaderSPIRVs {};
        ShaderReflectionData mReflectionData;
        Vector<std::function<void()>> mCallbacks;
    };
} // namespace Surge
