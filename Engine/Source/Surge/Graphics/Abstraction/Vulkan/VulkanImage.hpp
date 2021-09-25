// Copyright (c) - SurgeTechnologies - All rights reserved
//#include "Surge/Graphics/Image.hpp"
#include "Surge/Graphics/Texture.hpp"
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

    namespace Utils
    {
        VkDeviceSize CalculateImageBufferSize(Uint width, Uint height, ImageFormat imageFormat);

        VkFormat GetImageFormat(ImageFormat format);
        VkImageLayout GetImageLayoutUsage(ImageUsage usage);
        VkFilter GetImageFiltering(TextureFilter filtering);
        VkImageUsageFlags GetImageUsageFlags(ImageUsage usage);
        VkAccessFlags GetAccessFlagsFromLayout(VkImageLayout layout);
        VkPipelineStageFlags GetPipelineStagesFromLayout(VkImageLayout layout);

        void CreateImage(Uint width, Uint height, Uint texureDepth, Uint mipLevels,
                         VkFormat format, VkImageType type, VkImageTiling tiling,
                         VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
                         VkImage& image, VmaAllocation& imageMemory);

        void CreateImageView(VkImageView& imageView, VkImage& image, VkImageUsageFlags imageUsage, VkFormat format, Uint mipLevels, Uint textureDepth);
        void CreateImageSampler(VkFilter filtering, Uint mipLevels, VkSampler& sampler);

        void ChangeImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, Uint mipLevels, Uint depthMap);
        void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer& buffer, VkImage& image, Uint width, Uint height);

        void GenerateMipMaps(VkImage image, VkFormat imageFormat, VkImageLayout newLayout, int32_t texWidth, int32_t texHeight, Uint mipLevels);
    }

}