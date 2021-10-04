// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <volk.h>
#include <vk_mem_alloc.h>

namespace Surge
{
    class VulkanDevice;
    struct GPUMemoryStats;
    class VulkanMemoryAllocator
    {
    public:
        VulkanMemoryAllocator() = default;
        ~VulkanMemoryAllocator() = default;

        void Initialize(VkInstance instance, VulkanDevice& device);
        void Destroy();

        // Buffer
        VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer, VmaAllocationInfo* allocationInfo);
        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

        // Image
        VmaAllocation AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage, VmaAllocationInfo* allocationInfo);
        void DestroyImage(VkImage image, VmaAllocation allocation);

        void Free(VmaAllocation allocation);

        void* MapMemory(VmaAllocation allocation);
        void UnmapMemory(VmaAllocation allocation);

        GPUMemoryStats GetStats() const;
        VmaAllocator GetInternalAllocator() { return mAllocator; }

    private:
        VmaAllocator mAllocator;
    };
} // namespace Surge
