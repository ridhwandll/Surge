// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"

namespace Surge
{
    ShaderType VulkanUtils::ShaderTypeFromString(const String& type)
    {
        if (type == "Vertex]")  return ShaderType::Vertex;
        if (type == "Pixel]")   return ShaderType::Pixel;
        if (type == "Compute]") return ShaderType::Compute;

        return ShaderType::None;
    }

    String VulkanUtils::ShaderTypeToString(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::Vertex:  return "Vertex";
        case ShaderType::Pixel:   return "Pixel";
        case ShaderType::Compute: return "Compute";
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }
        return "";
    }

    shaderc_shader_kind VulkanUtils::ShadercShaderKindFromSurgeShaderType(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::Vertex:  return shaderc_glsl_vertex_shader;
        case ShaderType::Pixel:   return shaderc_glsl_fragment_shader;
        case ShaderType::Compute: return shaderc_glsl_compute_shader;
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }

        return static_cast<shaderc_shader_kind>(-1);
    }

    VkPrimitiveTopology VulkanUtils::GetVulkanPrimitiveTopology(PrimitiveTopology primitive)
    {
        switch (primitive)
        {
        case PrimitiveTopology::Points:         return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::Lines:          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::Triangles:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::None:           SG_ASSERT_INTERNAL("PrimitiveType::None is invalid!");
        }

        SG_ASSERT_INTERNAL("No Surge::PrimitiveType maps to VkPrimitiveTopology!");
        return VkPrimitiveTopology();
    }

    VkFormat VulkanUtils::ShaderDataTypeToVulkanFormat(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: SG_ASSERT_INTERNAL("Undefined!");
        }

        SG_ASSERT_INTERNAL("No Surge::ShaderDataType maps to VkFormat!");
        return VK_FORMAT_UNDEFINED;
    }

    Vector<VkDescriptorSetLayout> VulkanUtils::GetDescriptorSetLayoutVectorFromHashMap(const HashMap<Uint, VkDescriptorSetLayout>& descriptorSetLayouts)
    {
        Vector<VkDescriptorSetLayout> descriptorSetLayout;
        for (auto& layout : descriptorSetLayouts)
            descriptorSetLayout.push_back(layout.second);
        return descriptorSetLayout;
    }

    Vector<VkPushConstantRange> VulkanUtils::GetPushConstantRangesVectorFromHashMap(const HashMap<String, VkPushConstantRange>& pushConstants)
    {
        Vector<VkPushConstantRange> pushConstantsVector;
        for (auto& pushConstant : pushConstants)
            pushConstantsVector.push_back(pushConstant.second);
        return pushConstantsVector;
    }

    VkDescriptorType VulkanUtils::ShaderBufferTypeToVulkan(ShaderBuffer::Usage type)
    {
        switch (type)
        {
        case ShaderBuffer::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ShaderBuffer::Usage::Uniform: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        SG_ASSERT(false, "ShaderBuffer::Usage is invalid");
        return VkDescriptorType();
    }

    VkDescriptorType VulkanUtils::ShaderImageTypeToVulkan(ShaderResource::Usage type)
    {
        switch (type)
        {
        case ShaderResource::Usage::Sampled: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case ShaderResource::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        SG_ASSERT(false, "ShaderResource::Usage is invalid");
        return VkDescriptorType();
    }

    VkShaderStageFlags VulkanUtils::GetShaderStagesFlagsFromShaderTypes(const Vector<ShaderType>& shaderStages)
    {
        VkShaderStageFlags stageFlags{};
        for (const ShaderType& stage : shaderStages)
        {
            //None = 0, VertexShader, PixelShader, ComputeShader
            switch (stage)
            {
            case ShaderType::Vertex:  stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;   break;
            case ShaderType::Pixel:   stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT; break;
            case ShaderType::Compute: stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;  break;
            case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid!");
            }
        }
        return stageFlags;
    }

    void VulkanUtils::CreateWindowSurface(VkInstance instance, Window* windowHandle, VkSurfaceKHR* surface)
    {
    #ifdef SURGE_WINDOWS
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

        // Getting the vkCreateWin32SurfaceKHR function pointer and assert if it doesnt exist
        vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
        if (!vkCreateWin32SurfaceKHR)
            SG_ASSERT_INTERNAL("[Win32] Vulkan instance missing VK_KHR_win32_surface extension");

        VkWin32SurfaceCreateInfoKHR sci;
        memset(&sci, 0, sizeof(sci)); // Clear the info
        sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        sci.hinstance = GetModuleHandle(nullptr);
        sci.hwnd = static_cast<HWND>(windowHandle->GetNativeWindowHandle());

        VK_CALL(vkCreateWin32SurfaceKHR(instance, &sci, nullptr, surface));
    #else
        SG_ASSERT_INTERNAL("Surge is currently Windows Only! :(");
    #endif
    }

    void VulkanUtils::CreateImage(Uint width, Uint height, Uint texureDepth, Uint mipLevels,
        VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage, VkImage& outImage, VmaAllocation& outImageMemory)
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        VulkanMemoryAllocator* allocator = static_cast<VulkanMemoryAllocator*>(renderContext->GetMemoryAllocator());

        VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = type;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1; //TODO: Take in another parameter (for 3D textures?)
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = texureDepth;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = texureDepth == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; // If the image has 6 faces, then it is a cubemap
        outImageMemory = allocator->AllocateImage(imageInfo, memoryUsage, outImage, nullptr);
    }

    void VulkanUtils::CreateImageView(const VkImage& image, VkImageUsageFlags imageUsage, VkFormat format, Uint mipLevels, Uint textureDepth, VkImageView& outImageView)
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

        VK_CALL(vkCreateImageView(device, &createInfo, nullptr, &outImageView));
    }

    void VulkanUtils::CreateImageSampler(VkSamplerAddressMode adressMode, VkFilter filtering, Uint mipLevels, VkSampler& outSampler)
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* vulkanDevice = renderContext->GetDevice();
        VkDevice device = vulkanDevice->GetLogicalDevice();
        VkPhysicalDeviceProperties properties = vulkanDevice->GetPhysicalDeviceProperties();

        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = filtering;
        samplerInfo.minFilter = filtering;

        samplerInfo.addressModeU = adressMode;
        samplerInfo.addressModeV = adressMode;
        samplerInfo.addressModeW = adressMode;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        VK_CALL(vkCreateSampler(device, &samplerInfo, nullptr, &outSampler));
    }

    void VulkanUtils::ChangeImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, Uint mipLevels, Uint depthMap)
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* device = renderContext->GetDevice();

        VkImageAspectFlags aspectMask;
        if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        device->InstantSubmit(VulkanQueueType::Transfer, [&](VkCommandBuffer& cmd)
            {
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
                imageMemoryBarrier.srcAccessMask = VulkanUtils::GetAccessFlagsFromLayout(oldLayout);
                imageMemoryBarrier.dstAccessMask = VulkanUtils::GetAccessFlagsFromLayout(newLayout);

                VkPipelineStageFlags srcStageMask = GetPipelineStagesFromLayout(oldLayout);
                VkPipelineStageFlags dstStageMask = GetPipelineStagesFromLayout(newLayout);

                vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            });
    }

    void VulkanUtils::CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer& buffer, VkImage& image, Uint width, Uint height)
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

        vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void VulkanUtils::GenerateMipMaps(VkImage image, VkFormat imageFormat, VkImageLayout newLayout, int32_t texWidth, int32_t texHeight, Uint mipLevels)
    {
        VulkanRenderContext* renderContext = nullptr; SURGE_GET_VULKAN_CONTEXT(renderContext);
        VulkanDevice* device = renderContext->GetDevice();
        VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();


        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            SG_ASSERT_INTERNAL("Linear blitting not supported!");

        device->InstantSubmit(VulkanQueueType::Transfer, [&](VkCommandBuffer& cmd)
            {
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

                    VulkanUtils::InsertImageMemoryBarrier(cmd, image,
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

                    vkCmdBlitImage(cmd,
                        image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit,
                        VK_FILTER_LINEAR
                    );

                    VulkanUtils::InsertImageMemoryBarrier(cmd, image,
                        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newLayout,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        subresourceRange
                    );

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                subresourceRange.baseMipLevel = mipLevels - 1;

                VulkanUtils::InsertImageMemoryBarrier(cmd, image,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    subresourceRange
                );

            });
    }

    VkDeviceSize VulkanUtils::CalculateImageBufferSize(Uint width, Uint height, ImageFormat imageFormat)
    {
        switch (imageFormat)
        {
        case ImageFormat::RGBA8:     return width * height * sizeof(float);
        case ImageFormat::RGBA16F:   return width * height * sizeof(float) * 4;
        case ImageFormat::RGBA32F:   return width * height * sizeof(float) * 4;
        case ImageFormat::None:      SG_ASSERT_INTERNAL("ImageFormat::None is invalid!");
        }
        SG_ASSERT_INTERNAL("No matching ImageFormat found!")
        return 0;
    }

    VkFormat VulkanUtils::GetImageFormat(ImageFormat format)
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
        SG_ASSERT_INTERNAL("Invalid ImageFormat!");
        return VK_FORMAT_UNDEFINED;
    }

    VkImageLayout VulkanUtils::GetImageLayoutFromUsage(ImageUsage usage)
    {
        switch (usage)
        {
        case ImageUsage::Attachment:    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageUsage::DepthStencil:  return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case ImageUsage::Storage:       return VK_IMAGE_LAYOUT_GENERAL;
        case ImageUsage::None:          SG_ASSERT_INTERNAL("ImageUsage::None is invalid!");
        }
        SG_ASSERT_INTERNAL("Invalid ImageUsage!");
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkFilter VulkanUtils::GetImageFiltering(TextureFilter filtering)
    {
        switch (filtering)
        {
        case TextureFilter::Linear:   return VK_FILTER_LINEAR;
        case TextureFilter::Nearest:  return VK_FILTER_NEAREST;
        }
        SG_ASSERT_INTERNAL("Invalid TextureFilter!");
        return VK_FILTER_MAX_ENUM;
    }

    VkSamplerAddressMode VulkanUtils::GetImageAddressMode(TextureAddressMode wrap)
    {
        switch (wrap)
        {
        case TextureAddressMode::Repeat:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureAddressMode::ClampToBorder:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TextureAddressMode::ClampToEdge:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureAddressMode::MirroredRepeat:  return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
        SG_ASSERT_INTERNAL("Invalid TextureAddressMode!");
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }

    VkImageUsageFlags VulkanUtils::GetImageUsageFlags(ImageUsage usage)
    {
        switch (usage)
        {
        case ImageUsage::Attachment:    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        case ImageUsage::DepthStencil:  return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        case ImageUsage::Storage:       return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        case ImageUsage::None:          SG_ASSERT_INTERNAL("ImageUsage::None is invalid!");
        }
        SG_ASSERT_INTERNAL("Invalid image ImageUsage")
        return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
    }

    VkAccessFlags VulkanUtils::GetAccessFlagsFromLayout(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                   return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:             return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:             return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:         return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:         return VK_ACCESS_SHADER_READ_BIT;
        }
        return VK_ACCESS_NONE_KHR;
    }

    VkPipelineStageFlags VulkanUtils::GetPipelineStagesFromLayout(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:             return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:         return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:         return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                   return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_UNDEFINED:                        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    void VulkanUtils::InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkImageSubresourceRange subresourceRange)
    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.srcAccessMask = srcAccessMask;
        imageMemoryBarrier.dstAccessMask = dstAccessMask;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    VkShaderStageFlagBits VulkanUtils::GetVulkanShaderStage(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::Vertex:  return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::Pixel:   return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }
        return VkShaderStageFlagBits();
    }
}
