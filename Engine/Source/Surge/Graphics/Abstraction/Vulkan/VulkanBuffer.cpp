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
            CreateIndexBuffer(data);
            break;
        case BufferType::UniformBuffer:
            CreateUniformBuffer(data);
            break;
        case BufferType::None:
            SG_ASSERT_INTERNAL("BufferType must not be none!");
            break;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(CoreGetRenderContext()->GetMemoryAllocator());
        allocator->DestroyBuffer(mVulkanBuffer, mAllocation);
    }

    void VulkanBuffer::SetData(const void* data, const Uint& size)
    {
        memcpy(mAllocationInfo.pMappedData, data, size);
    }

    void VulkanBuffer::CreateVertexBuffer(const void* data)
    {
        Scope<RenderContext>& context = CoreGetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(context->GetInteralDevice());
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

        VkBufferCreateInfo vertexBufferCreateInfo = {};
        vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferCreateInfo.size = mSize;
        vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertexBufferCreateInfo.flags = VK_SHARING_MODE_EXCLUSIVE;
        mAllocation = allocator->AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mVulkanBuffer, &mAllocationInfo);

        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

        device->BeginCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);
        VkBufferCopy copyRegion = {};
        copyRegion.size = mSize;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, mVulkanBuffer, 1, &copyRegion);
        device->EndCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }

    void VulkanBuffer::CreateIndexBuffer(const void* data)
    {
        Scope<RenderContext>& context = CoreGetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(context->GetInteralDevice());
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

    void VulkanBuffer::CreateUniformBuffer(const void* data)
    {
        Scope<RenderContext>& context = CoreGetRenderContext();
        VulkanDevice* device = static_cast<VulkanDevice*>(context->GetInteralDevice());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(context->GetMemoryAllocator());

        VkBufferCreateInfo uniformBufferCreateInfo = {};
        uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        uniformBufferCreateInfo.size = mSize;
        uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniformBufferCreateInfo.flags = VK_SHARING_MODE_EXCLUSIVE;
        mAllocation = allocator->AllocateBuffer(uniformBufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, mVulkanBuffer, &mAllocationInfo);
    }
}
