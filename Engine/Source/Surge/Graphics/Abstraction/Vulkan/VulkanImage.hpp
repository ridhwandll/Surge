// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/Interface/Image.hpp"

namespace Surge
{
    class SURGE_API VulkanImage2D : public Image2D
    {
    public:
        VulkanImage2D(const ImageSpecification& specification);
        virtual ~VulkanImage2D();

        virtual Uint GetWidth() const override { return mSpecification.Width; }
        virtual Uint GetHeight() const override { return mSpecification.Height; }
        virtual void Release() override;
        virtual ImageSpecification& GetSpecification() override { return mSpecification; }
        virtual const ImageSpecification& GetSpecification() const override { return mSpecification; }

        // Vulkan Specific
        VkImage& GetVulkanImage() { return mImage; }
        VkImageView& GetVulkanImageView() { return mImageView; }
        VkSampler& GetVulkanSampler() { return mImageSampler; }
        VkImageLayout GetVulkanImageLayout() { return mDescriptorInfo.imageLayout; }
        const VkDescriptorImageInfo& GetVulkanDescriptorImageInfo() const { return mDescriptorInfo; }

    private:
        void Invalidate();
        void UpdateDescriptor();

    private:
        ImageSpecification mSpecification;

        VkImage mImage = VK_NULL_HANDLE;
        VkImageView mImageView = VK_NULL_HANDLE;
        VkSampler mImageSampler = VK_NULL_HANDLE;
        VmaAllocation mImageMemory;

        VkDescriptorImageInfo mDescriptorInfo;
        friend class SURGE_API VulkanTexture2D;
    };
} // namespace Surge
