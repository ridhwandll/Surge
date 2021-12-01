// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/Framebuffer.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferSpecification& spec);
        virtual ~VulkanFramebuffer() override;

        virtual void Resize(Uint width, Uint height) override;

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) const override;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) const override;

        virtual const FramebufferSpecification& GetSpecification() const override { return mSpecification; }
        virtual const Ref<Image2D>& GetColorAttachment(Uint index) const override { return mColorAttachmentImages[index]; }
        virtual const Ref<Image2D>& GetDepthAttachment() const override { return mDepthAttachmentImage; }

        // VulkanSpecific
        VkFramebuffer& GetVulkanFramebuffer() { return mFramebuffer; }
        VkRenderPass& GetVulkanRenderPass() { return mRenderPass; }

    private:
        void Invalidate();
        void Clear();

    private:
        FramebufferSpecification mSpecification;
        Vector<Ref<Image2D>> mColorAttachmentImages;
        Ref<Image2D> mDepthAttachmentImage;

        VkRenderPass mRenderPass = VK_NULL_HANDLE;
        VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    };
} // namespace Surge
