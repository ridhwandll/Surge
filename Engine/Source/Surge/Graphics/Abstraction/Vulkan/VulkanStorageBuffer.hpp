// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/StorageBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanStorageBuffer : public StorageBuffer
    {
    public:
        VulkanStorageBuffer(Uint size);
        virtual ~VulkanStorageBuffer() override;

        virtual void SetData(const void* data, Uint offset = 0) const override;
        virtual void SetData(const Buffer& data, Uint offset = 0) const override;
        virtual Uint GetSize() const override { return mSize; }
        virtual void Resize(Uint newSize) override;

        const VkBuffer& GetVulkanBuffer() const { return mVulkanBuffer; }
        const VkDescriptorBufferInfo& GetVulkanDescriptorBufferInfo() const { return mDescriptorInfo; }

    private:
        void Invalidate();
        void Release();

    private:
        Uint mSize;

        VkBuffer mVulkanBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
        VkDescriptorBufferInfo mDescriptorInfo {};
    };
} // namespace Surge