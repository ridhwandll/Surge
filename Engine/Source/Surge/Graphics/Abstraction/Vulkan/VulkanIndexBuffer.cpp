// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanIndexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    VulkanIndexBuffer::VulkanIndexBuffer(const void* data, const Uint& size)
        : mSize(size)
    {
        CreateIndexBuffer(data);
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        Scope<RenderContext>& renderContext = CoreGetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(renderContext->GetInternalDevice());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        vkDeviceWaitIdle(device->GetLogicaldevice());
        allocator->DestroyBuffer(mVulkanBuffer, mAllocation);
    }

    void VulkanIndexBuffer::Bind(const Ref<RenderCommandBuffer>& cmdBuffer)
    {
        VkDeviceSize offset = 0;
        Uint frameIndex = CoreGetRenderContext()->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        vkCmdBindIndexBuffer(vulkanCmdBuffer, mVulkanBuffer, 0, VK_INDEX_TYPE_UINT32);;
    }

    void VulkanIndexBuffer::CreateIndexBuffer(const void* data)
    {
        Scope<RenderContext>& context = CoreGetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(context->GetInternalDevice());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(context->GetMemoryAllocator());

        VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = mSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingBufferAllocation = allocator->AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, nullptr);

        // Copy data to staging buffer
        void* destData = allocator->MapMemory(stagingBufferAllocation);
        memcpy(destData, data, mSize);
        allocator->UnmapMemory(stagingBufferAllocation);

        VkBufferCreateInfo indexBufferCreateInfo = {};
        indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferCreateInfo.size = mSize;
        indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBufferCreateInfo.flags = VK_SHARING_MODE_EXCLUSIVE;
        mAllocation = allocator->AllocateBuffer(indexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mVulkanBuffer, &mAllocationInfo);

        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

        device->BeginCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);
        VkBufferCopy copyRegion = {};
        copyRegion.size = mSize;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, mVulkanBuffer, 1, &copyRegion);
        device->EndCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }
}
