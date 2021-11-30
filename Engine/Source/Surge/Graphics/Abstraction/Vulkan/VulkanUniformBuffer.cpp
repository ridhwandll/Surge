// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"
#include "VulkanRenderContext.hpp"

namespace Surge
{
    VulkanUniformBuffer::VulkanUniformBuffer(Uint size)
        : mSize(size)
    {
        Invalidate();
    }

    VulkanUniformBuffer::~VulkanUniformBuffer()
    {
        Release();
    }

    void VulkanUniformBuffer::SetData(const Buffer& data, Uint offset) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanMemoryAllocator* allocator = renderContext->GetMemoryAllocator();

        void* mappedData = allocator->MapMemory(mAllocation);
        memcpy(mappedData, (const Byte*)data.Data + offset, mSize);
        allocator->UnmapMemory(mAllocation);
    }

    void VulkanUniformBuffer::SetData(const void* data, Uint offset) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanMemoryAllocator* allocator = renderContext->GetMemoryAllocator();

        void* mappedData = allocator->MapMemory(mAllocation);
        memcpy(mappedData, (const Byte*)data + offset, mSize);
        allocator->UnmapMemory(mAllocation);
    }

    void VulkanUniformBuffer::Invalidate()
    {
        Release();
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = 0;
        allocInfo.memoryTypeIndex = 0;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.size = mSize;

        mAllocation = renderContext->GetMemoryAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, mVulkanBuffer, nullptr);
        SET_VK_OBJECT_DEBUGNAME(mVulkanBuffer, VK_OBJECT_TYPE_BUFFER, "Uniform Buffer");

        // Update Descriptor info
        mDescriptorInfo.buffer = mVulkanBuffer;
        mDescriptorInfo.range = mSize;
        mDescriptorInfo.offset = 0;
    }

    void VulkanUniformBuffer::Release()
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