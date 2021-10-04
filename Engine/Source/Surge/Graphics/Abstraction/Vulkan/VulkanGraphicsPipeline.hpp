// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/GraphicsPipeline.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const GraphicsPipelineSpecification& pipelineSpec);
        virtual ~VulkanGraphicsPipeline();

        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const override;
        virtual void SetPushConstantData(const Ref<RenderCommandBuffer>& cmdBuffer, const String& bufferName, void* data) const override;
        virtual void DrawIndexed(const Ref<RenderCommandBuffer>& cmdBuffer, Uint indicesCount, Uint baseIndex, Uint baseVertex) const override;

        VkPipeline GetVulkanPipeline() const { return mPipeline; }
        VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
        virtual const GraphicsPipelineSpecification& GetPipelineSpecification() const override { return mSpecification; }

    private:
        VkPipeline mPipeline;
        VkPipelineLayout mPipelineLayout;
        GraphicsPipelineSpecification mSpecification;
    };
} // namespace Surge