// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanVertexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    VulkanVertexBuffer::VulkanVertexBuffer(const void* data, const Uint& size) : mSize(size) { CreateVertexBuffer(data); }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        vkDeviceWaitIdle(device);
        allocator->DestroyBuffer(mVulkanBuffer, mAllocation);
    }

    void VulkanVertexBuffer::Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const
    {
        VkDeviceSize offset = 0;
        Uint frameIndex = SurgeCore::GetRenderContext()->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        vkCmdBindVertexBuffers(vulkanCmdBuffer, 0, 1, &mVulkanBuffer, &offset);
    }

    void VulkanVertexBuffer::CreateVertexBuffer(const void* data)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* device = renderContext->GetDevice();
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        VkBufferCreateInfo bufferCreateInfo {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
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

        device->InstantSubmit(VulkanQueueType::Transfer, [&](VkCommandBuffer& cmd) {
            VkBufferCopy copyRegion = {};
            copyRegion.size = mSize;
            vkCmdCopyBuffer(cmd, stagingBuffer, mVulkanBuffer, 1, &copyRegion);
        });

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }
} // namespace Surge
