// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>

//TODO: Remove
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    void Renderer::Initialize()
    {
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(true);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);

        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();

        mData->CubeMesh = Ref<Mesh>::Create("Engine/Assets/Mesh/Cube.fbx");
    }

    void Renderer::RenderRectangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());

        struct FrameData
        {
            glm::mat4 ViewProjection;
            glm::mat4 Transform;
        } uFrameData;
        uFrameData.ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;

        // Begin command buffer recording
        const Ref<RenderCommandBuffer>& cmd = mData->RenderCmdBuffer;
        mData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        mData->CubeMesh->GetPipeline()->Bind(cmd);
        mData->CubeMesh->GetVertexBuffer()->Bind(cmd);
        mData->CubeMesh->GetIndexBuffer()->Bind(cmd);

        const Submesh* submeshes = mData->CubeMesh->GetSubmeshes().data();
        for (Uint i = 0; i < mData->CubeMesh->GetSubmeshes().size(); i++)
        {
            const Submesh& submesh = submeshes[i];

            const glm::mat4 rot = glm::toMat4(glm::quat(rotation));
            uFrameData.Transform = (glm::translate(glm::mat4(1.0f), position) * rot * glm::scale(glm::mat4(1.0f), scale)) * submesh.Transform;
            mData->CubeMesh->GetPipeline()->SetPushConstantData(cmd, "uFrameData", (void*)&uFrameData);
            mData->CubeMesh->GetPipeline()->DrawIndexed(cmd, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
        }

        vkContext->RenderImGui();

        swapchain->EndRenderPass();
        mData->RenderCmdBuffer->EndRecording();
    }

    void Renderer::Shutdown()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(device);

        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void Renderer::BeginFrame(const EditorCamera& camera)
    {
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
    }

    void Renderer::EndFrame()
    {
        //TODO
    }

    Ref<Shader>& Renderer::GetShader(const String& name)
    {
        Ref<Shader>& result = mData->ShaderSet.GetShader(name);
        return result;
    }
}
