// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer.hpp"
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/GraphicsPipeline.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Abstraction/Vulkan/VulkanDevice.hpp"
#include "Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Abstraction/Vulkan/VulkanVertexBuffer.hpp"

namespace Surge
{
    // TODO: Very temporary, move initialization to something like "RendererBase"
    struct RendererData
    {
        Ref<VertexBuffer> VertexBuffer;
        Ref<IndexBuffer> IndexBuffer;
        Ref<GraphicsPipeline> Pipeline;
        Ref<RenderCommandBuffer> RenderCmdBuffer;
    };

    // TODO: This should be per Renderer Instance!
    static Scope<RendererData> sData = CreateScope<RendererData>();

    // TODO: Remove this
    struct Vertex
    {
        glm::vec3 Pos;
    };

    const std::vector<Vertex> vertices =
    {
        {{ -0.5f, -0.5f, 0.0f }},
        {{  0.5f, -0.5f, 0.0f }},
        {{  0.5f,  0.5f, 0.0f }},
        {{ -0.5f,  0.5f, 0.0f }}
    };
    const std::vector<Uint> indices = { 0, 1, 2, 2, 3, 0 };

    void Renderer::Initialize()
    {
        mBase.Initialize();
        sData->VertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<Uint>(sizeof(Vertex) * vertices.size()));
        sData->IndexBuffer = IndexBuffer::Create(indices.data(), static_cast<Uint>(sizeof(indices[0]) * indices.size()));

        GraphicsPipelineSpecification pipelineSpec{};
        pipelineSpec.Shader = mBase.GetShader("Simple");
        pipelineSpec.Topology = PrimitiveTopology::TriangleStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "TestPipeline";
        sData->Pipeline = GraphicsPipeline::Create(pipelineSpec);

        sData->RenderCmdBuffer = RenderCommandBuffer::Create(true);
    }

    void Renderer::RenderRectangle(const glm::vec3& color)
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());
        Uint imageIndex = vkContext->GetFrameIndex();

        VkCommandBuffer cmd = sData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(imageIndex);
        VkExtent2D extent = swapchain->GetVulkanExtent2D();

        // Begin command buffer recording
        sData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        scissor.offset = { 0, 0 };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdSetLineWidth(cmd, 5.0f);

        sData->Pipeline->Bind(sData->RenderCmdBuffer);
        sData->VertexBuffer->Bind(sData->RenderCmdBuffer);
        sData->IndexBuffer->Bind(sData->RenderCmdBuffer);
        sData->Pipeline->SetPushConstantData(sData->RenderCmdBuffer, "PushConstants", (void*)&color);
        vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);

        vkContext->RenderImGui();

        swapchain->EndRenderPass();
        sData->RenderCmdBuffer->EndRecording();
    }

    void Renderer::Shutdown()
    {
        sData.reset();
        mBase.Shutdown();
    }
}
