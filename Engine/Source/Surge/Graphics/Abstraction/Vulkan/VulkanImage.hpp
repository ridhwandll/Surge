// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Texture.hpp"
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

        void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout,
                              VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        void GenerateMipMaps(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout);

        VkImage GetVulkanImage() const { return mImage; }
        VkImageView GetVulkanImageView() const { return mImageView; }
        VkImageLayout GetVulkanImageLayout() const { return mImageLayout; }
        VkDescriptorImageInfo GetVulkanDescriptorInfo() const { return mDescriptorInfo; }

        virtual ImageSpecification& GetSpecification() override { return mImageSpecification; }
        virtual const ImageSpecification& GetSpecification() const override { return mImageSpecification; }
    private:
        void UpdateDescriptor();
    private:
        VkImage mImage;
        VkImageView mImageView;
        VkImageLayout mImageLayout;
        VmaAllocation mImageMemory;

        VkSampler mImageSampler;
        ImageSpecification mImageSpecification;

        VkDescriptorImageInfo mDescriptorInfo;
    };
}
