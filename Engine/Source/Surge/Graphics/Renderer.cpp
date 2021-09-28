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
        mData->mAllShaders.emplace_back(Shader::Create(BASE_SHADER_PATH"Simple.glsl"));
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(true);
        mData->CubeMesh = Ref<Mesh>::Create("Engine/Assets/Mesh/Cube.fbx");
    }

    void Renderer::RenderRectangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
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
            uFrameData.Transform = (glm::translate(glm::mat4(1.0f), position) * rot * glm::scale(glm::mat4(1.0f), scale));// * submesh.Transform;
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
        vkDestroyDescriptorPool(device, mData->DescriptorPool, nullptr);

        mData->mAllShaders.clear();
        mData.reset();
    }

    Ref<Shader>& Renderer::GetShader(const String& name)
    {
        for (Ref<Shader>& shader : mData->mAllShaders)
        {
            if (Filesystem::RemoveExtension(shader->GetPath()) == String(BASE_SHADER_PATH + name))
                return shader;
        }

        SG_ASSERT_INTERNAL("No shaders found with name: {0}", name);
        return mData->mDummyShader;
    }
}
