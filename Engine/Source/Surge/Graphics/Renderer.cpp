// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer.hpp"
#include "Shader.hpp"
#include "Buffer.hpp"
#include "GraphicsPipeline.hpp"
#include "Abstraction/Vulkan/VulkanDevice.hpp"
#include "Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include <array>
#include "Abstraction/Vulkan/VulkanBuffer.hpp"
#include "RenderCommandBuffer.hpp"
#include "Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    // TODO: Very temporary, move initialization to something like "RendererBase"
    struct RendererData
    {
        Ref<Buffer> VertexBuffer;
        Ref<Buffer> IndexBuffer;
        Ref<GraphicsPipeline> Pipeline;
        Ref<RenderCommandBuffer> RenderCmdBuffer;
    };

    // TODO: This should be per Renderer Instance!
    static Scope<RendererData> sData = CreateScope<RendererData>();

    // TODO: Remove this
    struct Vertex
    {
        glm::vec3 Pos;
        glm::vec3 Color;
    };

    const std::vector<Vertex> vertices =
    {
        {{-0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
        {{ 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
        {{ 0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        {{-0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }}
    };
    const std::vector<Uint> indices = { 0, 1, 2, 2, 3, 0 };

    void Renderer::Initialize()
    {
        mBase.Initialize();
        sData->VertexBuffer = Buffer::Create(vertices.data(), static_cast<Uint>(sizeof(Vertex) * vertices.size()), BufferType::VertexBuffer);
        sData->IndexBuffer = Buffer::Create(indices.data(), static_cast<Uint>(sizeof(indices[0]) * indices.size()), BufferType::IndexBuffer);

        GraphicsPipelineSpecification pipelineSpec{};
        pipelineSpec.Shader = mBase.GetShader("Simple");
        pipelineSpec.Topology = PrimitiveTopology::TriangleStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "TestPipeline";
        sData->Pipeline = GraphicsPipeline::Create(pipelineSpec);

        sData->RenderCmdBuffer = RenderCommandBuffer::Create(1, true); //Create command buffer from swapchain
    }

    void Renderer::RenderDatDamnTriangle()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->mDevice.GetLogicaldevice();
        VkSwapchainKHR swapChain = vkContext->mSwapChain.GetVulkanSwapChain();
        VkRenderPass renderPass = vkContext->mSwapChain.GetVulkanRenderPass();
        Uint imageIndex = vkContext->mSwapChain.GetCurrentFrameIndex();

        VkImageView swapChainImageView = vkContext->mSwapChain.GetVulkanImageViews()[imageIndex];
        VkExtent2D extent = vkContext->mSwapChain.GetVulkanExtent2D();

        VkRenderPassAttachmentBeginInfo attachmentInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO };
        attachmentInfo.attachmentCount = 1;
        attachmentInfo.pAttachments = &swapChainImageView;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = vkContext->mSwapChain.GetVulkanFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { extent.width, extent.height };
        renderPassInfo.pNext = &attachmentInfo; // Imageless framebuffer

        std::array<VkClearValue, 2> vkClearValues;
        vkClearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
        vkClearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
        renderPassInfo.pClearValues = vkClearValues.data();

        VkCommandBuffer cmd = sData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(imageIndex);
        sData->RenderCmdBuffer->BeginRecording();
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = (float)extent.width;
        viewport.height = (float)extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        scissor.offset = { 0, 0 };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sData->Pipeline.As<VulkanGraphicsPipeline>()->GetVulkanPipeline());
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdSetLineWidth(cmd, 5.0f);
        const VkBuffer vertexBuffer = sData->VertexBuffer.As<VulkanBuffer>()->GetVulkanBuffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        const VkBuffer indexBuffer = sData->IndexBuffer.As<VulkanBuffer>()->GetVulkanBuffer();
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);;
        vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
        vkCmdEndRenderPass(cmd);
        sData->RenderCmdBuffer->EndRecording();
    }

    void Renderer::Shutdown()
    {
        sData.reset();
        mBase.Shutdown();
    }
}
