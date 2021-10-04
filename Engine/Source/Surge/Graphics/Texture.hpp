// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Image.hpp"

namespace Surge
{
    struct TextureSpecification
    {
        ImageFormat Format = ImageFormat::None;
        ImageUsage Usage = ImageUsage::Texture;
        SamplerProperties Sampler {};
        bool UseMips = false;
    };

    class Texture : public RefCounted
    {
    public:
        virtual ~Texture() {}

        virtual Uint GetWidth() const = 0;
        virtual Uint GetHeight() const = 0;

        static Uint CalculateMipChainLevels(Uint width, Uint height);
        virtual TextureSpecification& GetSpecification() = 0;
        virtual const TextureSpecification& GetSpecification() const = 0;
    };

    class Texture2D : public Texture
    {
    public:
        virtual const Ref<Image2D> GetImage2D() const = 0;
        static Ref<Texture2D> Create(const String& filepath, TextureSpecification specification = {});
    };

} // namespace Surge