// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanTexture.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include <stb_image.h>

namespace Surge
{
    VulkanTexture2D::VulkanTexture2D(const String& filepath, TextureSpecification specification) : mFilePath(filepath), mSpecification(specification)
    {
        // Loading the texture
        int width, height, channels;
        ImageFormat imageFormat;
        bool defaultFormat = false;
        if (specification.Format == ImageFormat::None)
            defaultFormat = true;
        else
        {
            imageFormat = specification.Format;
            defaultFormat = false;
        }

        if (stbi_is_hdr(filepath.c_str()))
        {
            mPixelData = (void*)stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (defaultFormat)
                imageFormat = ImageFormat::RGBA32F;
        }
        else
        {
            mPixelData = (void*)stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (defaultFormat)
                imageFormat = ImageFormat::RGBA8;
        }
        SG_ASSERT(mPixelData, "Failed to load image!");
        mWidth = width;
        mHeight = height;
        mPixelDataSize = VulkanUtils::GetMemorySize(imageFormat, width, height);
        Uint mipChainLevels = CalculateMipChainLevels(width, height);

        // Creating the image
        ImageSpecification imageSpec {};
        imageSpec.Format = imageFormat;
        imageSpec.Width = mWidth;
        imageSpec.Height = mHeight;
        imageSpec.Mips = specification.UseMips ? mipChainLevels : 1;
        imageSpec.Usage = specification.Usage;
        imageSpec.Sampler = specification.Sampler;
        mImage = Image2D::Create(imageSpec);
        mSpecification.Format = imageFormat;

        Invalidate();
        stbi_image_free(mPixelData);
    }

    VulkanTexture2D::VulkanTexture2D(ImageFormat format, Uint width, Uint height, void* data, TextureSpecification specification)
    {
        mWidth = width;
        mHeight = height;
        mPixelData = data;
        mPixelDataSize = VulkanUtils::GetMemorySize(format, width, height);

        // Creating the image
        ImageSpecification imageSpec {};
        imageSpec.Format = format;
        imageSpec.Width = mWidth;
        imageSpec.Height = mHeight;
        imageSpec.Mips = 1;
        imageSpec.Usage = specification.Usage;
        imageSpec.Sampler = specification.Sampler;
        mImage = Image2D::Create(imageSpec);
        mSpecification.Format = format;

        Invalidate();
    }

    VulkanTexture2D::~VulkanTexture2D()
    {
        if (mImage)
            mImage->Release();
    }

    void VulkanTexture2D::Invalidate()
    {
        mImage->Release();
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(Core::GetRenderContext());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VulkanDevice* device = renderContext->GetDevice();

        ImageSpecification& imageSpec = mImage->GetSpecification();
        imageSpec.Format = mSpecification.Format;
        Ref<VulkanImage2D> image = mImage.As<VulkanImage2D>();
        image->Invalidate(); // ReCreate the image

        VkDeviceSize size = mPixelDataSize;

        // Create staging buffer
        VkBufferCreateInfo bufferCreateInfo {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation = allocator->AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer, nullptr);

        // Copy data to staging buffer
        void* destData = allocator->MapMemory(stagingBufferAllocation);
        SG_ASSERT(mPixelData, "Invalid pixel data!");
        memcpy(destData, mPixelData, mPixelDataSize);
        allocator->UnmapMemory(stagingBufferAllocation);

        device->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd) {
            VkImageSubresourceRange subresourceRange {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = imageSpec.Mips;
            subresourceRange.layerCount = 1;

            // Transition the texture image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            // VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            VkImageMemoryBarrier imageMemoryBarrier {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = image->GetVulkanImage();
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = mWidth;
            bufferCopyRegion.imageExtent.height = mHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // Copy data from staging buffer, the image is in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            vkCmdCopyBufferToImage(cmd, stagingBuffer, image->GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

            if (!mSpecification.UseMips)
            {
                // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                VulkanUtils::InsertImageMemoryBarrier(cmd, image->GetVulkanImage(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                      subresourceRange);
            }
            else
            {
                // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                // Note: Later transition-ed to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL in GenerateMips();
                VulkanUtils::InsertImageMemoryBarrier(cmd, image->GetVulkanImage(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, subresourceRange);
            }
        });
        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);

        if (mSpecification.UseMips)
            GenerateMips();
    }

    void VulkanTexture2D::GenerateMips()
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* device = renderContext->GetDevice();
        VkDevice logicalDevice = device->GetLogicalDevice();

        Ref<VulkanImage2D> image = mImage.As<VulkanImage2D>();
        ImageSpecification& imageSpec = mImage->GetSpecification();

        device->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd) {
            VkImage& vulkanImage = image->GetVulkanImage();
            VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.image = vulkanImage;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            for (Uint i = 1; i < imageSpec.Mips; i++)
            {
                VkImageBlit imageBlit {};

                // Source
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = int32_t(mWidth >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(mHeight >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                // Destination
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = int32_t(mWidth >> i);
                imageBlit.dstOffsets[1].y = int32_t(mHeight >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                // Prepare current mip level as image blit destination
                VulkanUtils::InsertImageMemoryBarrier(cmd, vulkanImage,
                                                      0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);

                // Blit from previous level
                vkCmdBlitImage(cmd, vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                // Prepare current mip level as image blit source for next level
                VulkanUtils::InsertImageMemoryBarrier(cmd, vulkanImage,
                                                      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);
            }

            // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.layerCount = 1;
            subresourceRange.levelCount = imageSpec.Mips;

            VulkanUtils::InsertImageMemoryBarrier(cmd, vulkanImage,
                                                  VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, subresourceRange);
        });
    }
} // namespace Surge