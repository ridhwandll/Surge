// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "VulkanImage.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"

namespace Surge
{
    VulkanImage2D::VulkanImage2D(const ImageSpecification& specification)
        : mImageSpecification(specification), mImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanDevice* device = renderContext->GetDevice();

        VkFormat textureFormat = VulkanUtils::GetImageFormat(specification.Format);
        VkImageUsageFlags usageFlags = VulkanUtils::GetImageUsageFlags(specification.Usage);
        VkImageLayout newImageLayout = VulkanUtils::GetImageLayoutUsage(specification.Usage);

        // Creating the image and allocating the needed buffer
        // (for creation we use `VK_IMAGE_LAYOUT_UNDEFINED` layout, which will be later changed to the user's input")
        VulkanUtils::CreateImage(specification.Width, specification.Height, 1, specification.Mips,
            textureFormat, VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,
            usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // Need the trasnfer src/dst bits for generating the mips
            VMA_MEMORY_USAGE_GPU_ONLY,
            mImage, mImageMemory);

        // Recording a temporary commandbuffer for transitioning
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        device->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd)
            {
                // Generating mip maps if the mip count is higher than 1, else just transition to user's input `VkImageLayout`
                if (specification.Mips > 1)
                {
                    TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VulkanUtils::GetPipelineStagesFromLayout(mImageLayout), VulkanUtils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
                    GenerateMipMaps(cmdBuffer, newImageLayout);
                }
                else
                {
                    TransitionLayout(cmdBuffer, newImageLayout, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VulkanUtils::GetPipelineStagesFromLayout(newImageLayout));
                }

            });

        // Creating the image view
        VulkanUtils::CreateImageView(mImage, usageFlags, textureFormat, specification.Mips, 1, mImageView);

        // Creating the sampler
        VkFilter filtering = VulkanUtils::GetImageFiltering(specification.Sampler.SamplerFilter);
        VulkanUtils::CreateImageSampler(filtering, specification.Mips, mImageSampler);

        // Updating the descriptor
        UpdateDescriptor();
    }

    VulkanImage2D::VulkanImage2D(const ImageSpecification& specification, const void* data)
        : mImageSpecification(specification), mImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VulkanDevice* device = renderContext->GetDevice();

        Uint imageSize = VulkanUtils::CalculateImageBufferSize(specification.Width, specification.Height, specification.Format);

        VkFormat textureFormat = VulkanUtils::GetImageFormat(specification.Format);
        VkImageUsageFlags usageFlags = VulkanUtils::GetImageUsageFlags(specification.Usage);
        VkImageLayout newImageLayout = VulkanUtils::GetImageLayoutUsage(specification.Usage);

        // Allocating a temporary buffer for copying the data into the image buffer
        VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferMemory;
        stagingBufferMemory = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer, nullptr);

        // Copying the data
        void* dstData = allocator->MapMemory(stagingBufferMemory);
        memcpy(dstData, data, static_cast<size_t>(imageSize));
        allocator->UnmapMemory(stagingBufferMemory);

        // Creating the image
        VulkanUtils::CreateImage(specification.Width, specification.Height, 1, specification.Mips, textureFormat, VK_IMAGE_TYPE_2D,
                           VK_IMAGE_TILING_OPTIMAL,
                           usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           VMA_MEMORY_USAGE_GPU_ONLY,
                           mImage, mImageMemory);

        // Recording a temporary into a temporary commandbuffer for transitioning/copying/generating mips
        device->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd)
            {
                // Changing the layout for copying the staging buffer to the iamge
                TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    VulkanUtils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
                VulkanUtils::CopyBufferToImage(cmd, stagingBuffer, mImage, specification.Width, specification.Height);

                // Generating mip maps if the mip count is higher than 1, else just transition to user's input `VkImageLayout`
                if (specification.Mips > 1)
                    GenerateMipMaps(cmd, newImageLayout);
                else
                    TransitionLayout(cmd, newImageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VulkanUtils::GetPipelineStagesFromLayout(newImageLayout));
            });

        // Creating the ImageView
        VulkanUtils::CreateImageView(mImage, usageFlags, textureFormat, specification.Mips, 1, mImageView);

        // Creating the sampler
        VkFilter filtering = VulkanUtils::GetImageFiltering(specification.Sampler.SamplerFilter);
        VulkanUtils::CreateImageSampler(filtering, specification.Mips, mImageSampler);

        // Updating the descriptor
        UpdateDescriptor();

        vkDestroyBuffer(device->GetLogicalDevice(), stagingBuffer, nullptr);
        allocator->Free(stagingBufferMemory);
    }

    VulkanImage2D::~VulkanImage2D()
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        vkDestroySampler(device, mImageSampler, nullptr);
        vkDestroyImageView(device, mImageView, nullptr);
        allocator->DestroyImage(mImage, mImageMemory);
    }

    void VulkanImage2D::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout,
                                         VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
    {
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mImageSpecification.Mips;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.oldLayout = mImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = mImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        imageMemoryBarrier.srcAccessMask = VulkanUtils::GetAccessFlagsFromLayout(mImageLayout);
        imageMemoryBarrier.dstAccessMask = VulkanUtils::GetAccessFlagsFromLayout(newImageLayout);


        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            cmdBuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier
        );

        mImageLayout = newImageLayout;
    }

    void VulkanImage2D::GenerateMipMaps(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout)
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanDevice* device = renderContext->GetDevice();
        VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();

        int32_t mipWidth = mImageSpecification.Width;
        int32_t mipHeight = mImageSpecification.Height;

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        subresourceRange.levelCount = 1;

        for (Uint i = 1; i < mImageSpecification.Mips; i++)
        {
            subresourceRange.baseMipLevel = i - 1;

            VulkanUtils::InsertImageMemoryBarrier(cmdBuffer, mImage,
                VulkanUtils::GetAccessFlagsFromLayout(mImageLayout), VulkanUtils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                mImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VulkanUtils::GetPipelineStagesFromLayout(mImageLayout), VulkanUtils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                subresourceRange
            );

            VkImageBlit blit{};

            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;

            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmdBuffer,
                mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            VulkanUtils::InsertImageMemoryBarrier(cmdBuffer, mImage,
                VulkanUtils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), VulkanUtils::GetAccessFlagsFromLayout(newImageLayout),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImageLayout,
                VulkanUtils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), VulkanUtils::GetPipelineStagesFromLayout(newImageLayout),
                subresourceRange
            );

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        subresourceRange.baseMipLevel = mImageSpecification.Mips - 1;

        VulkanUtils::InsertImageMemoryBarrier(cmdBuffer, mImage,
            VulkanUtils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), VulkanUtils::GetAccessFlagsFromLayout(newImageLayout),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newImageLayout,
            VulkanUtils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), VulkanUtils::GetPipelineStagesFromLayout(newImageLayout),
            subresourceRange
        );

        mImageLayout = newImageLayout;
    }

    void VulkanImage2D::UpdateDescriptor()
    {
        mDescriptorInfo.imageView = mImageView;
        mDescriptorInfo.imageLayout = mImageLayout;
        mDescriptorInfo.sampler = mImageSampler;
    }
}
