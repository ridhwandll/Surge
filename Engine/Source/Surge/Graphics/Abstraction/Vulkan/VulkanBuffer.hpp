// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Buffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanBuffer : public Buffer
    {
    public:
        VulkanBuffer() = default;
        VulkanBuffer(const void* data, const Uint& size, const BufferType& type);
        virtual ~VulkanBuffer() override;

        virtual Uint GetSize() override { return mSize; }
        virtual void SetData(const void* data, const Uint& size) override;
    public:
        VkBuffer GetVulkanBuffer() { return mVulkanBuffer; }
        VmaAllocation GetAllocation() { return mAllocation; }
    private:
        void CreateVertexBuffer(const void* data);
        void CreateIndexBuffer(const void* data);
        void CreateUniformBuffer(const void* data);
    private:
        Uint mSize = 0;
        BufferType mType = BufferType::None;
        VkBuffer mVulkanBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo mAllocationInfo = {};
    };
}
