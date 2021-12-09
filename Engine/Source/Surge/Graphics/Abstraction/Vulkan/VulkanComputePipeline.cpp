// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanComputePipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "VulkanUtils.hpp"

namespace Surge
{
    VulkanComputePipeline::VulkanComputePipeline(Ref<Shader>& computeShader)
        : mShader(computeShader.As<VulkanShader>()), mPipeline(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE)
    {
        Reload();
        mShaderReloadID = computeShader->AddReloadCallback([&]() { this->Reload(); });
    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {
        Release();
        mShader->RemoveReloadCallback(mShaderReloadID);
    }

    void VulkanComputePipeline::Bind(const Ref<RenderCommandBuffer>& renderCmdBuffer)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = renderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);

        vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
    }

    void VulkanComputePipeline::Dispatch(const Ref<RenderCommandBuffer>& renderCmdBuffer, Uint groupCountX, Uint groupCountY, Uint groupCountZ)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = renderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        vkCmdDispatch(vulkanCmdBuffer, groupCountX, groupCountY, groupCountZ);
    }

    void VulkanComputePipeline::Reload()
    {
        Release();
        SCOPED_TIMER("VulkanComputePipeline::Reload");

        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();

        HashMap<ShaderType, VkShaderModule> shaderModules = mShader->GetVulkanShaderModules();
        SG_ASSERT(shaderModules.size() == 1, "The Surge shader must only contain 1 shader, which is compute shader")
        VkPipelineShaderStageCreateInfo shaderStage {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        shaderStage.stage = VulkanUtils::GetVulkanShaderStage(ShaderType::Compute);
        shaderStage.module = shaderModules.at(ShaderType::Compute);
        shaderStage.pName = "main";
        shaderStage.pSpecializationInfo = nullptr;

        const Vector<VkDescriptorSetLayout> descriptorSetLayouts = VulkanUtils::GetDescriptorSetLayoutVectorFromMap(mShader->GetDescriptorSetLayouts());
        const Vector<VkPushConstantRange> pushConstants = VulkanUtils::GetPushConstantRangesVectorFromHashMap(mShader->GetPushConstantRanges());

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<Uint>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<Uint>(pushConstants.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants.data();
        VK_CALL(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));
        SET_VK_OBJECT_DEBUGNAME(mPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, "Compute PipelineLayout");

        VkComputePipelineCreateInfo computePipelineCreateInfo {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        computePipelineCreateInfo.layout = mPipelineLayout;
        computePipelineCreateInfo.flags = 0;
        computePipelineCreateInfo.stage = shaderStage;
        VK_CALL(vkCreateComputePipelines(logicalDevice, nullptr, 1, &computePipelineCreateInfo, nullptr, &mPipeline));
        SET_VK_OBJECT_DEBUGNAME(mPipeline, VK_OBJECT_TYPE_PIPELINE, "Compute Pipeline");
    }

    void VulkanComputePipeline::Release()
    {
        if (!mPipeline)
            return;

        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();

        vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
        vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
    }

} // namespace Surge
