// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"

#define VMA_IMPLEMENTATION
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "VulkanDiagnostics.hpp"

namespace Surge
{
    namespace Utils
    {
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    }

    void VulkanMemoryAllocator::Initialize(VkInstance instance, VulkanDevice& device)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorInfo.physicalDevice = device.GetPhysicalDevice();
        allocatorInfo.device = device.GetLogicalDevice();
        allocatorInfo.instance = instance;
        VK_CALL(vmaCreateAllocator(&allocatorInfo, &mAllocator));
    }

    void VulkanMemoryAllocator::Destroy()
    {
        vmaDestroyAllocator(mAllocator);
    }

    VmaAllocation VulkanMemoryAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer, VmaAllocationInfo* allocationInfo)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;
        allocCreateInfo.flags = allocationInfo ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;

        VmaAllocation allocation;
        vmaCreateBuffer(mAllocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, allocationInfo);

        return allocation;
    }

    void VulkanMemoryAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
    {
        SG_ASSERT_NOMSG(buffer);
        SG_ASSERT_NOMSG(allocation);
        vmaDestroyBuffer(mAllocator, buffer, allocation);
    }

	VmaAllocation VulkanMemoryAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage, VmaAllocationInfo* allocationInfo)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;
        allocCreateInfo.flags = allocationInfo ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;

        VmaAllocation allocation;
        vmaCreateImage(mAllocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, allocationInfo);

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

    void* VulkanMemoryAllocator::MapMemory(VmaAllocation allocation)
    {
        void* mappedMemory;
        vmaMapMemory(mAllocator, allocation, (void**)&mappedMemory);
        return mappedMemory;
    }

    void VulkanMemoryAllocator::UnmapMemory(VmaAllocation allocation)
    {
        vmaUnmapMemory(mAllocator, allocation);
    }

    GPUMemoryStats VulkanMemoryAllocator::GetStats() const
    {
        VmaStats stats;
        vmaCalculateStats(mAllocator, &stats);

        uint64_t usedMemory = stats.total.usedBytes;
        uint64_t freeMemory = stats.total.unusedBytes;

        return GPUMemoryStats(usedMemory, freeMemory);
    }

    namespace Utils
    {
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
            VkPhysicalDevice physicalDevice = renderContext->GetDevice()->GetPhysicalDevice();

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			SG_ASSERT(false, "Failed to find suitable memory type!");
			return 0;
		}
    }
}
