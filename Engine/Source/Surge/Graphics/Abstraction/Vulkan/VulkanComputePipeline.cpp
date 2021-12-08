// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanComputePipeline.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    VulkanComputePipeline::VulkanComputePipeline(Ref<Shader>& computeShader)
        : mShader(computeShader), mPipeline(VK_NULL_HANDLE), mPipelineLayout(VK_NULL_HANDLE)
    {
        Reload();
        mShaderReloadID = computeShader->AddReloadCallback([&]() { this->Reload(); });
    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {
        mShader->RemoveReloadCallback(mShaderReloadID);
    }

    void VulkanComputePipeline::Begin(const Ref<RenderCommandBuffer>& renderCmdBuffer)
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

    void VulkanComputePipeline::End(const Ref<RenderCommandBuffer>& renderCmdBuffer)
    {
        // TODO Submit to compute queue here
    }

    void VulkanComputePipeline::Reload()
    {
        Release();
        // TODO
    }

    void VulkanComputePipeline::Release()
    {
        // TODO
    }

} // namespace Surge
