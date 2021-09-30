// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Image.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanImage2D : public Image2D
    {
    public:
        VulkanImage2D(const ImageSpecification& specification);
        VulkanImage2D(const ImageSpecification& specification, const void* data);
        virtual ~VulkanImage2D();

        virtual Uint GetWidth() const override { return mImageSpecification.Width; }
        virtual Uint GetHeight() const override { return mImageSpecification.Height; }

        virtual ImageSpecification& GetSpecification() override { return mImageSpecification; }
        virtual const ImageSpecification& GetSpecification() const override { return mImageSpecification; }

        // Vulkan Specific
        VkImage GetVulkanImage() const { return mImage; }
        VkImageView GetVulkanImageView() const { return mImageView; }
        VkImageLayout GetVulkanImageLayout() const { return mImageLayout; }
        VkDescriptorImageInfo GetVulkanDescriptorInfo() const { return mDescriptorInfo; }
    private:
        void UpdateDescriptor();
        void GenerateMipMaps(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout);
        void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    private:
        ImageSpecification mImageSpecification;
        VkImageLayout mImageLayout;

        VkImage mImage;
        VkImageView mImageView;
        VkSampler mImageSampler;
        VmaAllocation mImageMemory;

        VkDescriptorImageInfo mDescriptorInfo;
    };
}
