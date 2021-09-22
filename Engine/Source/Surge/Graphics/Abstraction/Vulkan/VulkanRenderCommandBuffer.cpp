// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"

namespace Surge
{
    VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(bool createFromSwapchain, Uint size, const String& debugName)
        : mCreatedFromSwapchain(createFromSwapchain)
    {
        SCOPED_TIMER("[{0}] RenderCommandBuffer Creation", debugName);
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* vulkanDevice = renderContext->GetDevice();
        VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();


        if (!mCreatedFromSwapchain)
        {
            // Command Pool Creation
            VkCommandPoolCreateInfo cmdPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            cmdPoolInfo.queueFamilyIndex = vulkanDevice->GetQueueFamilyIndices().GraphicsQueue;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CALL(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &mCommandPool));

            // Command Buffers
            VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            commandBufferAllocateInfo.commandPool = mCommandPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = size;
            size == 0 ? mCommandBuffers.resize(FRAMES_IN_FLIGHT) : mCommandBuffers.resize(size);
            VK_CALL(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, mCommandBuffers.data()));

            // Sync Objects
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            mWaitFences.resize(size);
            for (VkFence& fence : mWaitFences)
            {
                VK_CALL(vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence));
            }
        }
        else
        {
            VulkanSwapChain* swapchain = renderContext->GetSwapChain();
            mCommandPool = swapchain->GetVulkanCommandPool();
            Vector<VkCommandBuffer>& swapchainCommandBuffers = swapchain->GetVulkanCommandBuffers();
            for (VkCommandBuffer& cmdBuffer : swapchainCommandBuffers)
                mCommandBuffers.push_back(cmdBuffer);
        }
    }

    VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
    {
        if (!mCreatedFromSwapchain)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
            vkDestroyCommandPool(device, mCommandPool, nullptr);
            for (VkFence& fence : mWaitFences)
            {
                vkDestroyFence(device, fence, nullptr);
            }
        }
    }

    void VulkanRenderCommandBuffer::BeginRecording()
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);

        Uint frameIndex = renderContext->GetFrameIndex();
        vkResetCommandBuffer(mCommandBuffers[frameIndex], 0);
        VkCommandBufferBeginInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkCommandBuffer commandBuffer = mCommandBuffers[frameIndex];
        vkBeginCommandBuffer(commandBuffer, &cmdBufInfo);
    }

    void VulkanRenderCommandBuffer::EndRecording()
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VK_CALL(vkEndCommandBuffer(mCommandBuffers[frameIndex]));
    }

    void VulkanRenderCommandBuffer::Submit()
    {
        if (mCreatedFromSwapchain)
            return;

        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* vulkanDevice = renderContext->GetDevice();
        VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();
        Uint frameIndex = renderContext->GetFrameIndex();

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer = mCommandBuffers[frameIndex];

        VK_CALL(vkWaitForFences(logicalDevice, 1, &mWaitFences[frameIndex], VK_TRUE, UINT64_MAX));
        VK_CALL(vkResetFences(logicalDevice, 1, &mWaitFences[frameIndex]));
        VK_CALL(vkQueueSubmit(vulkanDevice->GetGraphicsQueue(), 1, &submitInfo, mWaitFences[frameIndex]));
    }
}
