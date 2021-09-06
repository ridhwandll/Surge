// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/GraphicsPipeline.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const GraphicsPipelineSpecification& pipelineSpec);
        virtual ~VulkanGraphicsPipeline();
    
        virtual void Bind() override;
        void SetPushConstantData(void* data);

        VkPipeline GetVulkanPipeline() const { return mPipeline; }
        VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
        virtual const GraphicsPipelineSpecification& GetPipelineSpecification() const override { return mSpecification; }
    private:
        VkPipeline mPipeline;
        VkPipelineLayout mPipelineLayout;
        VkPushConstantRange mPushConstantRangeCache; // TODO(AC3R): Add push constants
        GraphicsPipelineSpecification mSpecification;
    };
}