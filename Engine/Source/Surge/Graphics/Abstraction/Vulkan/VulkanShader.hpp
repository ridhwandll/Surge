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
        virtual CallbackID AddReloadCallback(const std::function<void()> callback) override;
        virtual void RemoveReloadCallback(const CallbackID& id);
        virtual const ShaderReflectionData& GetReflectionData() const override { return mReflectionData; }
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const override { return mShaderSPIRVs; }
        virtual const Path& GetPath() const override { return mPath; }
        virtual const HashMap<ShaderType, String>& GetSources() const override { return mShaderSources; }
        virtual const HashCode& GetHash(const ShaderType& type) const override { return mHashCodes.at(type); }
        virtual const HashMap<ShaderType, HashCode>& GetHashCodes() const override { return mHashCodes; }

        // Vulkan Specific (Used by different Pipelines)
        const HashMap<ShaderType, VkShaderModule>& GetVulkanShaderModules() const { return mVkShaderModules; }
        const HashMap<Uint, VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
        const HashMap<String, VkPushConstantRange>& GetPushConstantRanges() const { return mPushConstants; }

    private:
        void ParseShader();
        void Compile(const HashMap<ShaderType, bool>& compileStages);
        void Clear();
        void CreateVulkanDescriptorSetLayouts();
        void CreateVulkanPushConstantRanges();

    private:
        Path mPath;
        HashMap<ShaderType, HashCode> mHashCodes;
        HashMap<ShaderType, String> mShaderSources;
        Vector<SPIRVHandle> mShaderSPIRVs {};

        HashMap<ShaderType, VkShaderModule> mVkShaderModules;
        HashMap<Uint, VkDescriptorSetLayout> mDescriptorSetLayouts;
        HashMap<String, VkPushConstantRange> mPushConstants;

        ShaderReflectionData mReflectionData;
        HashMap<CallbackID, std::function<void()>> mCallbacks;
    };

} // namespace Surge
