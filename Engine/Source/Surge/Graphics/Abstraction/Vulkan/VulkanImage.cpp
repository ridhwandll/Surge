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

        VkFormat textureFormat = Utils::GetImageFormat(specification.Format);
        VkImageUsageFlags usageFlags = Utils::GetImageUsageFlags(specification.Usage);
        VkImageLayout newImageLayout = Utils::GetImageLayoutUsage(specification.Usage);

        // Creating the image and allocating the needed buffer
        // (for creation we use `VK_IMAGE_LAYOUT_UNDEFINED` layout, which will be later changed to the user's input")
        Utils::CreateImage(specification.Width, specification.Height, 1, specification.Mips,
            textureFormat, VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,
            usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // Need the trasnfer src/dst bits for generating the mips
            VMA_MEMORY_USAGE_GPU_ONLY,
            mImage, mImageMemory);

        // Recording a temporary commandbuffer for transitioning
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        device->BeginOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Graphics);

        // Generating mip maps if the mip count is higher than 1, else just transition to user's input `VkImageLayout`
        if (specification.Mips > 1)
        {
            TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Utils::GetPipelineStagesFromLayout(mImageLayout), Utils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
            GenerateMipMaps(cmdBuffer, newImageLayout);
        }
        else
        {
            TransitionLayout(cmdBuffer, newImageLayout, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Utils::GetPipelineStagesFromLayout(newImageLayout));
        }

        // Ending the temporary commandbuffer 
        device->EndOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Graphics);


        // Creating the image view
        Utils::CreateImageView(mImageView, mImage, usageFlags, textureFormat, specification.Mips, 1);

        // Creating the sampler
        VkFilter filtering = Utils::GetImageFiltering(specification.Sampler.SamplerFilter);
        Utils::CreateImageSampler(filtering, specification.Mips, mImageSampler);

        // Updating the descriptor
        UpdateDescriptor();
    }

    VulkanImage2D::VulkanImage2D(const ImageSpecification& specification, const void* data)
        : mImageSpecification(specification), mImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());
        VulkanDevice* device = renderContext->GetDevice();

        Uint imageSize = Utils::CalculateImageBufferSize(specification.Width, specification.Height, specification.Format);

        VkFormat textureFormat = Utils::GetImageFormat(specification.Format);
        VkImageUsageFlags usageFlags = Utils::GetImageUsageFlags(specification.Usage);
        VkImageLayout newImageLayout = Utils::GetImageLayoutUsage(specification.Usage);


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
        Utils::CreateImage(specification.Width, specification.Height, 1, specification.Mips, textureFormat, VK_IMAGE_TYPE_2D,
                           VK_IMAGE_TILING_OPTIMAL,
                           usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           VMA_MEMORY_USAGE_GPU_ONLY,
                           mImage, mImageMemory);

        // Recording a temporary into a temporary commandbuffer for transitioning/copying/generating mips
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        device->BeginOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Graphics);

        // Changing the layout for copying the staging buffer to the iamge
        TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                                          Utils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
        Utils::CopyBufferToImage(cmdBuffer, stagingBuffer, mImage, specification.Width, specification.Height);

        // Generating mip maps if the mip count is higher than 1, else just transition to user's input `VkImageLayout`
        if (specification.Mips > 1)
            GenerateMipMaps(cmdBuffer, newImageLayout);
        else
            TransitionLayout(cmdBuffer, newImageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, Utils::GetPipelineStagesFromLayout(newImageLayout));

        // Ending the temporary commandbuffer
        device->EndOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Graphics);

        // Creating the ImageView
        Utils::CreateImageView(mImageView, mImage, usageFlags, textureFormat, specification.Mips, 1);

        // Creating the sampler
        VkFilter filtering = Utils::GetImageFiltering(specification.Sampler.SamplerFilter);
        Utils::CreateImageSampler(filtering, specification.Mips, mImageSampler);

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

        imageMemoryBarrier.srcAccessMask = Utils::GetAccessFlagsFromLayout(mImageLayout);
        imageMemoryBarrier.dstAccessMask = Utils::GetAccessFlagsFromLayout(newImageLayout);


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

            Utils::InsertImageMemoryBarrier(cmdBuffer, mImage,
                Utils::GetAccessFlagsFromLayout(mImageLayout), Utils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                mImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                Utils::GetPipelineStagesFromLayout(mImageLayout), Utils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
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

            Utils::InsertImageMemoryBarrier(cmdBuffer, mImage,
                Utils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), Utils::GetAccessFlagsFromLayout(newImageLayout),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImageLayout,
                Utils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), Utils::GetPipelineStagesFromLayout(newImageLayout),
                subresourceRange
            );

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        subresourceRange.baseMipLevel = mImageSpecification.Mips - 1;

        Utils::InsertImageMemoryBarrier(cmdBuffer, mImage,
            Utils::GetAccessFlagsFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), Utils::GetAccessFlagsFromLayout(newImageLayout),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newImageLayout,
            Utils::GetPipelineStagesFromLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), Utils::GetPipelineStagesFromLayout(newImageLayout),
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

    namespace Utils
    {
        void CreateImage(Uint width, Uint height, Uint texureDepth, Uint mipLevels,
                         VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
                         VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& imageMemory)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
            VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = type;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = texureDepth;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = texureDepth == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; // If the image has 6 faces, then it is a cubemap

            imageMemory = allocator->AllocateImage(imageInfo, memoryUsage, image, nullptr);
        }

        void CreateImageView(VkImageView& imageView, VkImage& image, VkImageUsageFlags imageUsage, VkFormat format, Uint mipLevels, Uint textureDepth)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
            
            VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            createInfo.image = image;
            createInfo.format = format;
            createInfo.viewType = textureDepth == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = mipLevels;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = textureDepth;
            switch (format)
            {
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
            default:
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VK_CALL(vkCreateImageView(device, &createInfo, nullptr, &imageView));
        }

        void CreateImageSampler(VkFilter filtering, Uint	 mipLevels, VkSampler& sampler)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
            VkPhysicalDevice physicalDevice = renderContext->GetDevice()->GetPhysicalDevice();

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = filtering;
            samplerInfo.minFilter = filtering;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(mipLevels);

            VK_CALL(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
        }

        void ChangeImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, Uint mipLevels, Uint depthMap)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VulkanDevice* device = renderContext->GetDevice();

            VkImageAspectFlags aspectMask;
            if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT)
                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            else
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;


            VkCommandBuffer cmdBuffer;
            device->BeginOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = aspectMask;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.layerCount = depthMap;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.oldLayout = oldLayout;
            imageMemoryBarrier.newLayout = newLayout;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = GetAccessFlagsFromLayout(oldLayout);
            imageMemoryBarrier.dstAccessMask = GetAccessFlagsFromLayout(newLayout);

            VkPipelineStageFlags srcStageMask = GetPipelineStagesFromLayout(oldLayout);
            VkPipelineStageFlags dstStageMask = GetPipelineStagesFromLayout(newLayout);

            vkCmdPipelineBarrier(
                cmdBuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier
            );

            device->EndOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);
        }

        void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer& buffer, VkImage& image, Uint width, Uint height)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VulkanDevice* device = renderContext->GetDevice();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.layerCount = 1;
            region.imageSubresource.baseArrayLayer = 0;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                width,
                height,
                1
            };

            vkCmdCopyBufferToImage(
                cmdBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        }

        void GenerateMipMaps(VkImage image, VkFormat imageFormat, VkImageLayout newLayout, int32_t texWidth, int32_t texHeight, Uint mipLevels)
        {
            VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
            VulkanDevice* device = renderContext->GetDevice();
            VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();


            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

            if (!(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
                SG_ASSERT_INTERNAL("Linear blitting not supported!");

            VkCommandBuffer cmdBuffer;
            device->BeginOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);


            int32_t mipWidth = texWidth;
            int32_t mipHeight = texHeight;

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;
            subresourceRange.levelCount = 1;


            for (Uint i = 1; i < mipLevels; i++)
            {
                subresourceRange.baseMipLevel = i - 1;

                Utils::InsertImageMemoryBarrier(cmdBuffer, image,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    subresourceRange
                );

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(cmdBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );

                Utils::InsertImageMemoryBarrier(cmdBuffer, image,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newLayout,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    subresourceRange
                );

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;

            }

            subresourceRange.baseMipLevel = mipLevels - 1;

            Utils::InsertImageMemoryBarrier(cmdBuffer, image,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                subresourceRange
            );

            device->EndOneTimeCmdBuffer(cmdBuffer, VulkanQueueType::Transfer);
        }

        VkDeviceSize CalculateImageBufferSize(Uint width, Uint height, ImageFormat imageFormat)
        {
            switch (imageFormat)
            {
            case ImageFormat::RGBA8:     return width * height * sizeof(float);
            case ImageFormat::RGBA16F:   return width * height * sizeof(float) * 4;
            case ImageFormat::RGBA32F:   return width * height * sizeof(float) * 4;
            case ImageFormat::None:      SG_ASSERT_INTERNAL("ImageFormat::None is invalid!");
            }
            return 0;
        }

        VkFormat GetImageFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA8:            return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA16F:          return VK_FORMAT_R16G16B16A16_SFLOAT;
            case ImageFormat::RGBA32F:          return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ImageFormat::Depth32:          return VK_FORMAT_D32_SFLOAT;
            case ImageFormat::Depth24Stencil8:  return VK_FORMAT_D24_UNORM_S8_UINT;
            case ImageFormat::None:             SG_ASSERT_INTERNAL("ImageFormat::None is invalid!");
            }
            return VK_FORMAT_UNDEFINED;
        }

        VkImageLayout GetImageLayoutUsage(ImageUsage usage)
        {
            switch (usage)
            {
            case ImageUsage::Storage:       return VK_IMAGE_LAYOUT_GENERAL;
            case ImageUsage::Attachment:    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case ImageUsage::DepthStencil:  return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            case ImageUsage::None:			SG_ASSERT_INTERNAL("ImageUsage::None is invalid!");
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkFilter GetImageFiltering(TextureFilter filtering)
        {
            switch (filtering)
            {
            case TextureFilter::Linear:   return VK_FILTER_LINEAR;
            case TextureFilter::Nearest:  return VK_FILTER_NEAREST;
            }
            return VK_FILTER_MAX_ENUM;
        }

        VkImageUsageFlags GetImageUsageFlags(ImageUsage usage)
        {
            switch (usage)
            {
            case ImageUsage::Storage:       return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            case ImageUsage::Attachment:    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            case ImageUsage::DepthStencil:  return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            case ImageUsage::None:          SG_ASSERT_INTERNAL("ImageUsage::None is invalid!");
            }
            return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
        }

        VkAccessFlags GetAccessFlagsFromLayout(VkImageLayout layout)
        {
            switch (layout)
            {
            case VK_IMAGE_LAYOUT_PREINITIALIZED:				   return VK_ACCESS_HOST_WRITE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:			   return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:			   return VK_ACCESS_TRANSFER_READ_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:		   return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:		   return VK_ACCESS_SHADER_READ_BIT;
            }
            return VK_ACCESS_NONE_KHR;
        }

        VkPipelineStageFlags GetPipelineStagesFromLayout(VkImageLayout layout)
        {
            switch (layout)
            {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:			   return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:		   return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:		   return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case VK_IMAGE_LAYOUT_PREINITIALIZED:				   return VK_PIPELINE_STAGE_HOST_BIT;
            case VK_IMAGE_LAYOUT_UNDEFINED:						   return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
    }
}