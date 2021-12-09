// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/ComputePipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanComputePipeline : public ComputePipeline
    {
    public:
        VulkanComputePipeline(Ref<Shader>& computeShader);
        virtual ~VulkanComputePipeline() override;

        void Bind(const Ref<RenderCommandBuffer>& renderCmdBuffer) override;
        void Dispatch(const Ref<RenderCommandBuffer>& renderCmdBuffer, Uint groupCountX, Uint groupCountY, Uint groupCountZ) override;
        const Ref<Shader>& GetShader() const override { return mShader; }

        VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }

    private:
        void Reload();
        void Release();

    private:
        Ref<VulkanShader> mShader;
        VkPipeline mPipeline;
        VkPipelineLayout mPipelineLayout;
        VkDescriptorSetLayout mEmptyLayout;
        CallbackID mShaderReloadID;
    };

} // namespace Surge
