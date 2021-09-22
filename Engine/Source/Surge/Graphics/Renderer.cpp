// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer.hpp"
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/GraphicsPipeline.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include <glm/gtc/matrix_transform.hpp>

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

    void Renderer::RenderRectangle(const glm::vec3& position)
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());
        VkExtent2D extent = swapchain->GetVulkanExtent2D();

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 1000.0f);
        projection[1][1] *= -1;

        struct FrameData
        {
            glm::mat4 ViewProjection;
            glm::mat4 Transform;
        } uFrameData;
        uFrameData.ViewProjection = projection * view;
        uFrameData.Transform = glm::translate(glm::mat4(1.0f), position);

        // Begin command buffer recording
        const Ref<RenderCommandBuffer>& cmd = sData->RenderCmdBuffer;
        sData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        sData->Pipeline->Bind(cmd);
        sData->VertexBuffer->Bind(cmd);
        sData->IndexBuffer->Bind(cmd);
        sData->Pipeline->SetPushConstantData(cmd, "uFrameData", (void*)&uFrameData);
        sData->Pipeline->DrawIndexed(cmd, indices.size());

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
