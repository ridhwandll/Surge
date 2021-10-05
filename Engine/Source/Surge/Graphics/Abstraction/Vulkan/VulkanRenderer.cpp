// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImGuiContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanFramebuffer.hpp"
#include <imgui.h> // TODO(AC3R): Temporary

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

        // Creating n descriptor pools for allocating descriptor sets by the application (n = frames in flight)
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        sDescriptorPool.resize(FRAMES_IN_FLIGHT);
        for (auto& descriptorPool : sDescriptorPool)
        {
            VkDescriptorPoolSize poolSizes[] =
                {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
                 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};

            VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = 100 * (sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            poolInfo.poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            poolInfo.pPoolSizes = poolSizes;
            VK_CALL(vkCreateDescriptorPool(renderContext->GetDevice()->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool));
        }
    }

    void VulkanRenderer::Shutdown()
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        vkDeviceWaitIdle(device);
        for (auto& descriptorPool : sDescriptorPool)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void VulkanRenderer::BeginFrame(const EditorCamera& camera)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        Uint currentFrameIndex = renderContext->GetFrameIndex();

        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->DrawList.clear();
    }

    void VulkanRenderer::EndFrame()
    {
        mData->RenderCmdBuffer->BeginRecording();

        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;
        BeginRenderPass(mData->RenderCmdBuffer, mData->OutputFrambuffer);
        for (const DrawCommand& object : mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.Mesh;
            const Ref<GraphicsPipeline>& graphicsPipeline = mesh->GetPipeline();
            graphicsPipeline->Bind(mData->RenderCmdBuffer);
            mesh->GetVertexBuffer()->Bind(mData->RenderCmdBuffer);
            mesh->GetIndexBuffer()->Bind(mData->RenderCmdBuffer);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                graphicsPipeline->SetPushConstantData(mData->RenderCmdBuffer, "uFrameData", &pushConstantData);
                graphicsPipeline->DrawIndexed(mData->RenderCmdBuffer, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }
        EndRenderPass(mData->RenderCmdBuffer);

        mData->RenderCmdBuffer->EndRecording();
        mData->RenderCmdBuffer->Submit();
    }

    void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
    {
        mData->DrawList.emplace_back(mesh, transform);
    }

    void VulkanRenderer::BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer)
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        const FramebufferSpecification& framebufferSpec = framebuffer->GetSpecification();

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {framebufferSpec.ClearColor.r, framebufferSpec.ClearColor.g, framebufferSpec.ClearColor.b, framebufferSpec.ClearColor.a};
        clearValues[1].depthStencil = {1.0f, 0};

        VkViewport viewport = {};
        viewport.width = float(framebufferSpec.Width);
        viewport.height = float(framebufferSpec.Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.extent = {framebufferSpec.Width, framebufferSpec.Height};
        scissor.offset = {0, 0};

        VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = framebuffer.As<VulkanFramebuffer>()->GetVulkanRenderPass();
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {framebufferSpec.Width, framebufferSpec.Height};
        renderPassBeginInfo.framebuffer = framebuffer.As<VulkanFramebuffer>()->GetVulkanFramebuffer();
        renderPassBeginInfo.clearValueCount = Uint(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdSetViewport(vulkanCmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(vulkanCmdBuffer, 0, 1, &scissor);
        vkCmdBeginRenderPass(vulkanCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer)
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        vkCmdEndRenderPass(cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex));
    }

    VkDescriptorSet VulkanRenderer::AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        Uint currentFrameIndex = renderContext->GetFrameIndex();

        VkDescriptorSet descriptorSet;
        allocInfo.descriptorPool = sDescriptorPool[currentFrameIndex];
        VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

        return descriptorSet;
    }

} // namespace Surge
