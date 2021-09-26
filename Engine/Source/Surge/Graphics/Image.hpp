// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"

namespace Surge
{
    enum class ImageFormat
    {
        None = 0,

        // Color
        RGBA8, RGBA16F, RGBA32F,

        // Depth/Stencil
        Depth32,
        Depth24Stencil8
    };

    enum class ImageUsage
    {
        None = 0,
        Storage,     // VK_GENERAL
        Attachment,  // VK_COLOR_ATTACHEMNT_OPTIMAL
        DepthStencil // VK_DEPTH_STENCIL_OPTIMAL
    };

    enum class TextureFilter
    {
        Linear, Nearest
    };

    enum class TextureWrap
    {
        Repeat
    };

    struct SamplerProperties
    {
        TextureWrap SamplerWrap = TextureWrap::Repeat;
        TextureFilter SamplerFilter = TextureFilter::Linear;
    };

    struct ImageSpecification
    {
        ImageFormat Format = ImageFormat::RGBA8;
        ImageUsage Usage = ImageUsage::Storage;
        SamplerProperties Sampler{};
        Vector<ShaderType> ShaderUsage;
        Uint Width = 0;
        Uint Height = 0;
        Uint Mips = 1;
    };

    class Image : public RefCounted
    {
    public:
        virtual ~Image() {}

        virtual Uint GetWidth() const = 0;
        virtual Uint GetHeight() const = 0;

        virtual ImageSpecification& GetSpecification() = 0;
        virtual const ImageSpecification& GetSpecification() const = 0;
    };

    class Image2D : public Image
    {
    public:
        static Ref<Image2D> Create(const ImageSpecification& specification);
        static Ref<Image2D> Create(const ImageSpecification& specification, const void* data);
    };
}
