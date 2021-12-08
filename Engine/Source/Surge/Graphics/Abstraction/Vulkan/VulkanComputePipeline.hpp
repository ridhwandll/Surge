// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/ComputePipeline.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanComputePipeline : public ComputePipeline
    {
    public:
        VulkanComputePipeline(Ref<Shader>& computeShader);
        virtual ~VulkanComputePipeline() override;

        void Begin(const Ref<RenderCommandBuffer>& renderCmdBuffer) override;
        void Dispatch(const Ref<RenderCommandBuffer>& renderCmdBuffer, Uint groupCountX, Uint groupCountY, Uint groupCountZ) override;
        void End(const Ref<RenderCommandBuffer>& renderCmdBuffer) override;
        const Ref<Shader>& GetShader() const override { return mShader; }

    private:
        void Reload();
        void Release();

    private:
        Ref<Shader> mShader;
        VkPipeline mPipeline;
        VkPipelineLayout mPipelineLayout;
        CallbackID mShaderReloadID;
    };

} // namespace Surge
