// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanIndexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"

namespace Surge
{
    VulkanIndexBuffer::VulkanIndexBuffer(const void* data, const Uint& size) : mSize(size) { CreateIndexBuffer(data); }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        vkDeviceWaitIdle(renderContext->GetDevice()->GetLogicalDevice());
        allocator->DestroyBuffer(mVulkanBuffer, mAllocation);
    }

    void VulkanIndexBuffer::Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkCommandBuffer vulkanCmdBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(renderContext->GetFrameIndex());
        vkCmdBindIndexBuffer(vulkanCmdBuffer, mVulkanBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanIndexBuffer::CreateIndexBuffer(const void* data)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VulkanDevice* vulkanDevice = renderContext->GetDevice();

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

        VkBufferCreateInfo indexBufferCreateInfo = {};
        indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferCreateInfo.size = mSize;
        indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBufferCreateInfo.flags = VK_SHARING_MODE_EXCLUSIVE;
        mAllocation = allocator->AllocateBuffer(indexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mVulkanBuffer, nullptr);

        vulkanDevice->InstantSubmit(VulkanQueueType::Transfer, [&](VkCommandBuffer& cmd) {
            VkBufferCopy copyRegion = {};
            copyRegion.size = mSize;
            vkCmdCopyBuffer(cmd, stagingBuffer, mVulkanBuffer, 1, &copyRegion);
        });

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
        SET_VK_OBJECT_DEBUGNAME(mVulkanBuffer, VK_OBJECT_TYPE_BUFFER, "Index Buffer");
    }
} // namespace Surge