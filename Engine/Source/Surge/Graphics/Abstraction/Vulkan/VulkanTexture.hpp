// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Texture.hpp"

namespace Surge
{
    class VulkanTexture2D : public Texture2D
    {
    public:
        VulkanTexture2D(const String& filepath, TextureSpecification specification);
        virtual ~VulkanTexture2D();

        virtual Uint GetWidth() const override { return mWidth; }
        virtual Uint GetHeight() const override { return mHeight; }

        virtual TextureSpecification& GetSpecification() override { return mSpecification; }
        virtual const TextureSpecification& GetSpecification() const override { return mSpecification; }

        virtual const Ref<Image2D> GetImage2D() const override { return mImage; }
    private:
        Ref<Image2D> mImage;
        TextureSpecification mSpecification;
        String mFilePath;

        Uint mWidth, mHeight;
        Uint mMipMaps;
    };
}