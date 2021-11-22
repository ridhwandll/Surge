// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/IndexBuffer.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer() = default;
        VulkanIndexBuffer(const void* data, const Uint& size);
        virtual ~VulkanIndexBuffer() override;

        virtual Uint GetSize() const override { return mSize; }

    public:
        const VkBuffer GetVulkanBuffer() const { return mVulkanBuffer; }
        VmaAllocation GetAllocation() { return mAllocation; }

    private:
        void CreateIndexBuffer(const void* data);

    private:
        Uint mSize = 0;
        VkBuffer mVulkanBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
    };
} // namespace Surge