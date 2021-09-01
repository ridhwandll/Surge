// Copyright (c) - SurgeTechnologies - All rights reserved
#define VMA_IMPLEMENTATION
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/RenderContext.hpp"
#include "VulkanDiagnostics.hpp"

namespace Surge
{
    void VulkanMemoryAllocator::Initialize(VkInstance instance, VulkanDevice& device)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorInfo.physicalDevice = device.GetPhysicaldevice();
        allocatorInfo.device = device.GetLogicaldevice();
        allocatorInfo.instance = instance;
        VK_CALL(vmaCreateAllocator(&allocatorInfo, &mAllocator));
    }

    void VulkanMemoryAllocator::Destroy()
    {
        vmaDestroyAllocator(mAllocator);
    }

    VmaAllocation VulkanMemoryAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

        VmaAllocation allocation;
        vmaCreateBuffer(mAllocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

        return allocation;
    }

    void VulkanMemoryAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
    {
        SG_ASSERT_NOMSG(buffer);
        SG_ASSERT_NOMSG(allocation);
        vmaDestroyBuffer(mAllocator, buffer, allocation);
    }

    VmaAllocation VulkanMemoryAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

        VmaAllocation allocation;
        vmaCreateImage(mAllocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);

        return allocation;
    }

    void VulkanMemoryAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
    {
        SG_ASSERT_NOMSG(image);
        SG_ASSERT_NOMSG(allocation);
        vmaDestroyImage(mAllocator, image, allocation);
    }

    void VulkanMemoryAllocator::Free(VmaAllocation allocation)
    {
        vmaFreeMemory(mAllocator, allocation);
    }

    GPUMemoryStats VulkanMemoryAllocator::GetStats() const
    {
        VmaStats stats;
        vmaCalculateStats(mAllocator, &stats);

        uint64_t usedMemory = stats.total.usedBytes;
        uint64_t freeMemory = stats.total.unusedBytes;

        return GPUMemoryStats(usedMemory, freeMemory);
    }
}