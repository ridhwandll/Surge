// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "VulkanSwapChain.hpp"

namespace Surge
{
    VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(Uint size, bool createFromSwapchain, const String& debugName)
        : mCreatedFromSwapchain(createFromSwapchain)
    {
        SCOPED_TIMER("[{0}] RenderCommandBuffer Creation", debugName);
        VulkanDevice* device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice());
        VkDevice logicalDevice = device->GetLogicaldevice();

        if (!mCreatedFromSwapchain)
        {
            // Command Pool Creation
            VkCommandPoolCreateInfo cmdPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            cmdPoolInfo.queueFamilyIndex = device->GetQueueFamilyIndices().GraphicsQueue;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CALL(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &mCommandPool));

            // Command Buffers
            VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            commandBufferAllocateInfo.commandPool = mCommandPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = size;
            if (size != 0) mCommandBuffers.resize(size);
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
            VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(CoreGetRenderContext()->GetSwapChain());
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
            VulkanDevice* device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice());
            VkDevice logicalDevice = device->GetLogicaldevice();
            vkDestroyCommandPool(logicalDevice, mCommandPool, nullptr);
            for (VkFence& fence : mWaitFences)
            {
                vkDestroyFence(logicalDevice, fence, nullptr);
            }
        }
    }

    void VulkanRenderCommandBuffer::BeginRecording()
    {
        Uint frameIndex = ((VulkanSwapChain*)(CoreGetRenderContext()->GetSwapChain()))->GetCurrentFrameIndex(); //TODO: More than two Frames in Flight
        //vkResetCommandBuffer(mCommandBuffers[frameIndex], 0);
        VkCommandBufferBeginInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkCommandBuffer commandBuffer = mCommandBuffers[frameIndex];
        vkBeginCommandBuffer(commandBuffer, &cmdBufInfo);
    }

    void VulkanRenderCommandBuffer::EndRecording()
    {
        Uint frameIndex = ((VulkanSwapChain*)(CoreGetRenderContext()->GetSwapChain()))->GetCurrentFrameIndex(); //TODO: More than two Frames in Flight
        VkCommandBuffer commandBuffer = mCommandBuffers[frameIndex];
        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void VulkanRenderCommandBuffer::Submit()
    {
        if (mCreatedFromSwapchain)
            return;

        VulkanDevice* device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice());
        VkDevice logicalDevice = device->GetLogicaldevice();
        Uint frameIndex = ((VulkanSwapChain*)(CoreGetRenderContext()->GetSwapChain()))->GetCurrentFrameIndex(); //TODO: More than two Frames in Flight

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer = mCommandBuffers[frameIndex];

        VK_CALL(vkWaitForFences(logicalDevice, 1, &mWaitFences[frameIndex], VK_TRUE, UINT64_MAX));
        VK_CALL(vkResetFences(logicalDevice, 1, &mWaitFences[frameIndex]));
        VK_CALL(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, mWaitFences[frameIndex]));
    }
}
