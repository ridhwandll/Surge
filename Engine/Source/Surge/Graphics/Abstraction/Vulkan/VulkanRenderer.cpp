// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "VulkanRenderContext.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/Framebuffer.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanImage.hpp"
#include "VulkanRenderCommandBuffer.hpp"
#include "VulkanShader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    void VulkanRenderer::Initialize()
    {
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(false);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);
        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();

        FramebufferSpecification spec = {};
        spec.Formats = {ImageFormat::RGBA8, ImageFormat::Depth32};
        spec.Width = 1280;
        spec.Height = 720;
        mData->OutputFrambuffer = Framebuffer::Create(spec);
    }

    void VulkanRenderer::Shutdown()
    {
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
        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;

        const Ref<RenderCommandBuffer>& cmd = mData->RenderCmdBuffer;
        mData->RenderCmdBuffer->BeginRecording();
        mData->OutputFrambuffer->BeginRenderPass(cmd);

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
        mData->OutputFrambuffer->EndRenderPass(cmd);
        mData->RenderCmdBuffer->EndRecording();
        mData->RenderCmdBuffer->Submit();
    }

    void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
    {
        mData->DrawList.emplace_back(mesh, transform);
    }
} // namespace Surge
