// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "VulkanRenderContext.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    void VulkanRenderer::Initialize()
    {
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(true);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);

        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();
    }

    void VulkanRenderer::Shutdown()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(device);

        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void VulkanRenderer::BeginFrame(const EditorCamera& camera)
    {
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->DrawList.clear();
    }

    void VulkanRenderer::EndFrame()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());

        const Ref<RenderCommandBuffer>& cmd = mData->RenderCmdBuffer;
        mData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;

        for (const DrawCommand& object : mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.Mesh;
            const Ref<GraphicsPipeline>& graphicsPipeline = mesh->GetPipeline();
            graphicsPipeline->Bind(cmd);
            mesh->GetVertexBuffer()->Bind(cmd);
            mesh->GetIndexBuffer()->Bind(cmd);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                graphicsPipeline->SetPushConstantData(cmd, "uFrameData", &pushConstantData);
                graphicsPipeline->DrawIndexed(cmd, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }

        vkContext->RenderImGui();
        swapchain->EndRenderPass();
        mData->RenderCmdBuffer->EndRecording();
    }

    void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
    {
        mData->DrawList.emplace_back(mesh, transform);
    }
}
