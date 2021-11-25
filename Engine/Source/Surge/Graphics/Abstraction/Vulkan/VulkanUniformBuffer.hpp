// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/UniformBuffer.hpp"
#include "Surge/Core/Buffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanUniformBuffer : public UniformBuffer
    {
    public:
        VulkanUniformBuffer(Uint size);
        virtual ~VulkanUniformBuffer() override;

        virtual void SetData(const void* data, Uint offset = 0) const override;
        virtual void SetData(const Buffer& data, Uint offset = 0) const override;
        virtual Uint GetSize() const override { return mSize; }

        const VkBuffer& GetVulkanBuffer() const { return mVulkanBuffer; }
        const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return mDescriptorInfo; }

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