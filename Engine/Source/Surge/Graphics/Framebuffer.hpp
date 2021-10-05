// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Image.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"

namespace Surge
{
    struct FramebufferSpecification
    {
        Uint Width = 0, Height = 0;
        Vector<ImageFormat> Formats;
        glm::vec4 ClearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    };

    class Framebuffer : public RefCounted
    {
    public:
        virtual ~Framebuffer() = default;

        virtual const FramebufferSpecification& GetSpecification() const = 0;
        virtual FramebufferSpecification& GetSpecification() = 0;
        virtual const Ref<Image2D>& GetColorAttachment(Uint index) const = 0;

        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };
} // namespace Surge
