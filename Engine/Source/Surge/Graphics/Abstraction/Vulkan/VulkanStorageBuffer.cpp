// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanStorageBuffer.hpp"
#include "VulkanRenderContext.hpp"

namespace Surge
{
    VulkanStorageBuffer::VulkanStorageBuffer(Uint size, GPUMemoryUsage memoryUsage)
        : mSize(size), mMemoryUsage(memoryUsage)
    {
        Invalidate();
    }

    VulkanStorageBuffer::~VulkanStorageBuffer()
    {
        Release();
    }

    void VulkanStorageBuffer::SetData(const Buffer& data, Uint offset) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanMemoryAllocator* allocator = renderContext->GetMemoryAllocator();

        void* mappedData = allocator->MapMemory(mAllocation);
        memcpy(mappedData, (const Byte*)data.Data + offset, mSize);
        allocator->UnmapMemory(mAllocation);
    }

    void VulkanStorageBuffer::SetData(const void* data, Uint offset) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanMemoryAllocator* allocator = renderContext->GetMemoryAllocator();

        void* mappedData = allocator->MapMemory(mAllocation);
        memcpy(mappedData, (const Byte*)data + offset, mSize);
        allocator->UnmapMemory(mAllocation);
    }

    void VulkanStorageBuffer::Resize(Uint newSize)
    {
        mSize = newSize;
        Invalidate();
    }

    void VulkanStorageBuffer::Invalidate()
    {
        Release();
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = 0;
        allocInfo.memoryTypeIndex = 0;

        VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfo.size = mSize;

        mAllocation = renderContext->GetMemoryAllocator()->AllocateBuffer(bufferInfo, SurgeMemoryUsageToVmaMemoryUsage(mMemoryUsage), mVulkanBuffer, nullptr);
        SET_VK_OBJECT_DEBUGNAME(mVulkanBuffer, VK_OBJECT_TYPE_BUFFER, "Storage Buffer");

        // Update Descriptor info
        mDescriptorInfo.buffer = mVulkanBuffer;
        mDescriptorInfo.range = mSize;
        mDescriptorInfo.offset = 0;
    }

    void VulkanStorageBuffer::Release()
    {
        if (!mAllocation)
            return;

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        vkDeviceWaitIdle(renderContext->GetDevice()->GetLogicalDevice());
        renderContext->GetMemoryAllocator()->DestroyBuffer(mVulkanBuffer, mAllocation);

        mVulkanBuffer = VK_NULL_HANDLE;
        mAllocation = VK_NULL_HANDLE;
    }

} // namespace Surge
