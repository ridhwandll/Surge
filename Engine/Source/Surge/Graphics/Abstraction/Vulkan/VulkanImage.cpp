// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"

namespace Surge
{
    VulkanImage2D::VulkanImage2D(const ImageSpecification& specification)
        : mSpecification(specification)
    {
        Invalidate();
    }

    void VulkanImage2D::Release()
    {
        if (mImage == VK_NULL_HANDLE)
            return;

        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        vkDestroyImageView(device, mImageView, nullptr);
        vkDestroySampler(device, mImageSampler, nullptr);
        allocator->DestroyImage(mImage, mImageMemory);

        mImage = VK_NULL_HANDLE;
        mImageView = VK_NULL_HANDLE;
        mImageSampler = VK_NULL_HANDLE;
    }

    void VulkanImage2D::Invalidate()
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanDevice* device = renderContext->GetDevice();
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        VkFormat textureFormat = VulkanUtils::GetImageFormat(mSpecification.Format);
        VkImageUsageFlags usageFlags = VulkanUtils::GetImageUsageFlags(mSpecification.Usage, mSpecification.Format);

        VkImageAspectFlags aspectMask = VulkanUtils::IsDepthFormat(mSpecification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (mSpecification.Format == ImageFormat::Depth24Stencil8)
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        VkImageCreateInfo imageInfo {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = mSpecification.Width;
        imageInfo.extent.height = mSpecification.Height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mSpecification.Mips;
        imageInfo.arrayLayers = 1;
        imageInfo.format = textureFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usageFlags;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        mImageMemory = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, mImage, nullptr);

        // Create the image view
        VkImageViewCreateInfo imageViewCreateInfo {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = imageInfo.format;
        imageViewCreateInfo.subresourceRange = {};
        imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = mSpecification.Mips;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.image = mImage;
        VK_CALL(vkCreateImageView(device->GetLogicalDevice(), &imageViewCreateInfo, nullptr, &mImageView));

        // Sampler
        VkSamplerCreateInfo samplerCreateInfo {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerCreateInfo.anisotropyEnable = VK_FALSE; // TODO: have an option to enable it
        samplerCreateInfo.magFilter = VulkanUtils::GetImageFiltering(mSpecification.Sampler.SamplerFilter);
        samplerCreateInfo.minFilter = samplerCreateInfo.magFilter;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VulkanUtils::GetImageAddressMode(mSpecification.Sampler.SamplerAddressMode);
        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = float(mSpecification.Mips);
        VK_CALL(vkCreateSampler(device->GetLogicalDevice(), &samplerCreateInfo, nullptr, &mImageSampler));

        // Transition image to VK_IMAGE_LAYOUT_GENERAL layout, if it is Storage
        if (mSpecification.Usage == ImageUsage::Storage)
        {
            device->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd) {
                VkImageSubresourceRange subresourceRange {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = mSpecification.Mips;
                subresourceRange.layerCount = 1;

                VulkanUtils::InsertImageMemoryBarrier(cmd, mImage, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subresourceRange);
            });
        }

        UpdateDescriptor();
    }

    void VulkanImage2D::UpdateDescriptor()
    {
        if (mSpecification.Format == ImageFormat::Depth24Stencil8 || mSpecification.Format == ImageFormat::Depth24Stencil8)
            mDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (mSpecification.Usage == ImageUsage::Storage)
            mDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        else
            mDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        mDescriptorInfo.imageView = mImageView;
        mDescriptorInfo.sampler = mImageSampler;
    }

    VulkanImage2D::~VulkanImage2D()
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(device);

        if (mImageSampler)
        {
            vkDestroySampler(device, mImageSampler, nullptr);
            mImageSampler = VK_NULL_HANDLE;
        }
        if (mImageView)
        {
            vkDestroyImageView(device, mImageView, nullptr);
            mImageView = VK_NULL_HANDLE;
        }
        if (mImage)
        {
            allocator->DestroyImage(mImage, mImageMemory);
            mImage = VK_NULL_HANDLE;
        }
    }
} // namespace Surge
