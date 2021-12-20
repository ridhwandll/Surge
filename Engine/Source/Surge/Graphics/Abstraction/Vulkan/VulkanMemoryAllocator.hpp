// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <volk.h>
#include <vk_mem_alloc.h>

namespace Surge
{
    class VulkanDevice;
    struct GPUMemoryStats;

    FORCEINLINE VmaMemoryUsage SurgeMemoryUsageToVmaMemoryUsage(GPUMemoryUsage usage)
    {
        switch (usage)
        {
            case GPUMemoryUsage::Unknown:
                return VMA_MEMORY_USAGE_UNKNOWN;
            case GPUMemoryUsage::GPUOnly:
                return VMA_MEMORY_USAGE_GPU_ONLY;
            case GPUMemoryUsage::CPUOnly:
                return VMA_MEMORY_USAGE_CPU_ONLY;
            case GPUMemoryUsage::CPUToGPU:
                return VMA_MEMORY_USAGE_CPU_TO_GPU;
            case GPUMemoryUsage::GPUToCPU:
                return VMA_MEMORY_USAGE_GPU_TO_CPU;
            case GPUMemoryUsage::CPUCopy:
                return VMA_MEMORY_USAGE_CPU_COPY;
            case GPUMemoryUsage::GPULazilyAllocated:
                return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
        }
        SG_ASSERT_INTERNAL("Invalid GPUMemoryUsage!");
        return VMA_MEMORY_USAGE_UNKNOWN;
    }

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
