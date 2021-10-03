// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Texture.hpp"

namespace Surge
{
    class VulkanTexture2D : public Texture2D
    {
    public:
        VulkanTexture2D(const String& filepath, TextureSpecification specification);
        virtual ~VulkanTexture2D() override;

        virtual Uint GetWidth() const override { return mWidth; }
        virtual Uint GetHeight() const override { return mHeight; }

        virtual TextureSpecification& GetSpecification() override { return mSpecification; }
        virtual const TextureSpecification& GetSpecification() const override { return mSpecification; }

        virtual const Ref<Image2D> GetImage2D() const override { return mImage; }
    private:
        void Invalidate();
        void GenerateMips();
    private:
        Ref<Image2D> mImage;
        TextureSpecification mSpecification;
        String mFilePath;
        Uint mWidth, mHeight;

        void* mPixelData;
        Uint mPixelDataSize;
    };
}
