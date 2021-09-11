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

        VkSemaphore mPresentSemaphore, mRenderSemaphore;
        VkFence mRenderFence;
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
        pipelineSpec.Topology = PrimitiveTopology::LineStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "TestPipeline";
        sData->Pipeline = GraphicsPipeline::Create(pipelineSpec);

        sData->RenderCmdBuffer = RenderCommandBuffer::Create(1);

        // TODO: Abstract the Renderer!
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());

        // Create the fence
        VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CALL(vkCreateFence(vkContext->mDevice.GetLogicaldevice(), &fenceCreateInfo, nullptr, &sData->mRenderFence));

        // Create the semaphores
        VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;
        VK_CALL(vkCreateSemaphore(vkContext->mDevice.GetLogicaldevice(), &semaphoreCreateInfo, nullptr, &sData->mPresentSemaphore));
        VK_CALL(vkCreateSemaphore(vkContext->mDevice.GetLogicaldevice(), &semaphoreCreateInfo, nullptr, &sData->mRenderSemaphore));
    }

    void Renderer::RenderDatDamnTriangle()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->mDevice.GetLogicaldevice();
        VkSwapchainKHR swapChain = vkContext->mSwapChain.GetVulkanSwapChain();
        VkRenderPass renderPass = vkContext->mSwapChain.GetVulkanRenderPass();

        //Wait until the GPU has finished rendering the last frame. Timeout of 1 second
        VK_CALL(vkWaitForFences(device, 1, &sData->mRenderFence, true, 1000000000));
        VK_CALL(vkResetFences(device, 1, &sData->mRenderFence));

        uint32_t imageIndex;
        VK_CALL(vkAcquireNextImageKHR(device, swapChain, 1000000000, sData->mPresentSemaphore, nullptr, &imageIndex));
        VkCommandBuffer cmd = sData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(0);
        sData->RenderCmdBuffer->BeginRecording();

        VkImageView swapChainImageView = vkContext->mSwapChain.GetImageViews()[imageIndex];
        VkExtent2D extent = vkContext->mSwapChain.GetExtent();

        VkCommandBufferBeginInfo cmdBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBeginInfo.pNext = nullptr;
        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkRenderPassAttachmentBeginInfo attachmentInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO };
        attachmentInfo.attachmentCount = 1;
        attachmentInfo.pAttachments = &swapChainImageView;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = vkContext->mSwapChain.GetFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { extent.width, extent.height };
        renderPassInfo.pNext = &attachmentInfo; // Imageless framebuffer

        std::array<VkClearValue, 2> vkClearValues;
        vkClearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
        vkClearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
        renderPassInfo.pClearValues = vkClearValues.data();

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

        // Command buffer Submission
        VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.pNext = nullptr;
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &waitStage;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &sData->mPresentSemaphore;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &sData->mRenderSemaphore;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        VK_CALL(vkQueueSubmit(vkContext->mDevice.GetGraphicsQueue(), 1, &submit, sData->mRenderFence));

        // Presentation
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.swapchainCount = 1;
        presentInfo.pWaitSemaphores = &sData->mRenderSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pImageIndices = &imageIndex;
        VK_CALL(vkQueuePresentKHR(vkContext->mDevice.GetGraphicsQueue(), &presentInfo));
    }

    void Renderer::Shutdown()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->mDevice.GetLogicaldevice();

        vkDeviceWaitIdle(device);
        vkDestroyFence(device, sData->mRenderFence, nullptr);
        vkDestroySemaphore(device, sData->mPresentSemaphore, nullptr);
        vkDestroySemaphore(device, sData->mRenderSemaphore, nullptr);

        sData.reset();
        mBase.Shutdown();
    }
}
