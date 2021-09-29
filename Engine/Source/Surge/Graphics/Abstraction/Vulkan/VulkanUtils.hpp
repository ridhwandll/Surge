// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/ReflectionData.hpp"
#include "Surge/Graphics/Image.hpp"
#include <shaderc/shaderc.h>

namespace Surge::VulkanUtils
{
    ShaderType ShaderTypeFromString(const String& type);
    String ShaderTypeToString(const ShaderType& type);
    shaderc_shader_kind ShadercShaderKindFromSurgeShaderType(const ShaderType& type);

    VkPrimitiveTopology GetVulkanPrimitiveTopology(PrimitiveTopology primitive);
    VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type);
    Vector<VkDescriptorSetLayout> GetDescriptorSetLayoutVectorFromHashMap(const HashMap<Uint, VkDescriptorSetLayout>& descriptorSetLayouts);
    Vector<VkPushConstantRange> GetPushConstantRangesVectorFromHashMap(const HashMap<String, VkPushConstantRange>& pushConstants);
    VkDescriptorType ShaderBufferTypeToVulkan(ShaderBuffer::Usage type);
    VkDescriptorType ShaderImageTypeToVulkan(ShaderResource::Usage type);
    VkShaderStageFlags GetShaderStagesFlagsFromShaderTypes(const Vector<ShaderType>& shaderStages);
    void CreateWindowSurface(VkInstance instance, Window* windowHandle, VkSurfaceKHR* surface);
    VkShaderStageFlagBits GetVulkanShaderStage(ShaderType type);
    VkCompareOp GetVulkanCompareOp(CompareOperation op);
    VkPolygonMode GetVulkanPolygonMode(PolygonMode mode);
    VkCullModeFlags GetVulkanCullModeFlags(CullMode mode);

    // Image Related
    VkDeviceSize CalculateImageBufferSize(Uint width, Uint height, ImageFormat imageFormat);
    VkFormat GetImageFormat(ImageFormat format);
    VkImageLayout GetImageLayoutFromUsage(ImageUsage usage);
    VkFilter GetImageFiltering(TextureFilter filtering);
    VkSamplerAddressMode GetImageAddressMode(TextureAddressMode wrap);
    VkImageUsageFlags GetImageUsageFlags(ImageUsage usage);
    VkAccessFlags GetAccessFlagsFromLayout(VkImageLayout layout);
    VkPipelineStageFlags GetPipelineStagesFromLayout(VkImageLayout layout);

    void CreateImage(Uint width, Uint height, Uint texureDepth, Uint mipLevels,
        VkFormat format, VkImageType type, VkImageTiling tiling,
        VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
        VkImage& outImage, VmaAllocation& outImageMemory);

    void CreateImageView(const VkImage& image, VkImageUsageFlags imageUsage, VkFormat format, Uint mipLevels, Uint textureDepth, VkImageView& outImageView);
    void CreateImageSampler(VkSamplerAddressMode adressMode, VkFilter filtering, Uint mipLevels, VkSampler& outSampler);

    void ChangeImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, Uint mipLevels, Uint depthMap);
    void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer& buffer, VkImage& image, Uint width, Uint height);

    void GenerateMipMaps(VkImage image, VkFormat imageFormat, VkImageLayout newLayout, int32_t texWidth, int32_t texHeight, Uint mipLevels);

    void InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkImageSubresourceRange subresourceRange);
}
