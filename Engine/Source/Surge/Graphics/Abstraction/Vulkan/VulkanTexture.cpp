// Copyright (c) - SurgeTechnologies - All rights reserved
#include "VulkanTexture.hpp"

#include <stb_image.h>

namespace Surge
{
    VulkanTexture2D::VulkanTexture2D(const String& filepath, TextureSpecification specification)
        : mFilePath(filepath), mSpecification(specification)
    {
        // Loading the texture
        int width, height, channels;
        ImageFormat imageFormat;
        void* pixels = nullptr;
        if (stbi_is_hdr(filepath.c_str()))
        {
            pixels = (void*)stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            imageFormat = ImageFormat::RGBA32F;
        }
        else
        {
            pixels = (void*)stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            imageFormat = ImageFormat::RGBA8;
        }
        SG_ASSERT(pixels, "Failed to load image!");

        // Getting the mip levels
        Uint mipChainLevels = CalculateMipChainLevels(width, height);

        // Creating the image
        ImageSpecification imageSpec{};
        imageSpec.Format = imageFormat;
        imageSpec.Width = width;
        imageSpec.Height = height;
        imageSpec.Mips = specification.UseMips ? mipChainLevels : 1;
        imageSpec.ShaderUsage = specification.ShaderUsage;
        imageSpec.Usage = specification.Usage;
        imageSpec.Sampler.SamplerFilter = TextureFilter::Nearest;
        imageSpec.Sampler.SamplerWrap = TextureWrap::Repeat;
        mImage = Image2D::Create(imageSpec, pixels);
    }
}
