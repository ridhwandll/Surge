// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"

namespace Surge
{
    VulkanBuffer::VulkanBuffer(const void* data, const Uint& size, const BufferType& type)
        : mSize(size), mType(type)
    {
        switch (type)
        {
        case BufferType::VertexBuffer:
            CreateVertexBuffer(data);
            break;
        case BufferType::IndexBuffer:
            SG_ASSERT_INTERNAL("Not Implemented!");
            break;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(GetRenderContext()->GetMemoryAllocator());
        allocator->DestroyBuffer(mVulkanBuffer, mAllocation);
    }

    void VulkanBuffer::CreateVertexBuffer(const void* data)
    {
        Scope<RenderContext>& context = GetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(context->GetInteralDevice());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(context->GetMemoryAllocator());

        VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = mSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingBufferAllocation = allocator->AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

        // Copy data to staging buffer
        void* destData = allocator->MapMemory(stagingBufferAllocation);
        memcpy(destData, data, mSize);
        allocator->UnmapMemory(stagingBufferAllocation);

        VkBufferCreateInfo vertexBufferCreateInfo = {};
        vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferCreateInfo.size = mSize;
        vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        mAllocation = allocator->AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mVulkanBuffer);

        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

        device->BeginCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);
        VkBufferCopy copyRegion = {};
        copyRegion.size = mSize;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, mVulkanBuffer, 1, &copyRegion);
        device->EndCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }
}
