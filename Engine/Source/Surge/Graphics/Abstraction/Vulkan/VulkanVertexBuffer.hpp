// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
        VulkanVertexBuffer() = default;
        VulkanVertexBuffer(const void* data, const Uint& size);
        virtual ~VulkanVertexBuffer() override;

        virtual Uint GetSize() override { return mSize; }

        const VkBuffer GetVulkanBuffer() const { return mVulkanBuffer; }
        VmaAllocation GetAllocation() { return mAllocation; }

    private:
        void CreateVertexBuffer(const void* data);

    private:
        Uint mSize = 0;
        VkBuffer mVulkanBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
    };
} // namespace Surge
