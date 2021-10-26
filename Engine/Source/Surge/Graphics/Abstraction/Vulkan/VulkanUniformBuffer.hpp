// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/UniformBuffer.hpp"
#include "Surge/Core/Buffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanUniformBuffer : public UniformBuffer
    {
    public:
        VulkanUniformBuffer(Uint size, Uint binding);
        virtual ~VulkanUniformBuffer() override;

        virtual void SetData(const Buffer& data, Uint offset = 0) const override;
        virtual const Buffer& GetData() const { return mDataBuffer; }
        virtual Uint GetSize() const override { return mSize; }
        virtual Uint GetBinding() const override { return mBinding; }

        const VkBuffer& GetVulkanBuffer() const { return mVulkanBuffer; }
        const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return mDescriptorInfo; }

    private:
        void Invalidate();
        void Release();

    private:
        Uint mSize;
        Uint mBinding;
        Buffer mDataBuffer;

        VkBuffer mVulkanBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
        VkDescriptorBufferInfo mDescriptorInfo {};
    };

} // namespace Surge
