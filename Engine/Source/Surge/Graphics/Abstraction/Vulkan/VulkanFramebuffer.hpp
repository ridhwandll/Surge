// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Framebuffer.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(const FramebufferSpecification& spec);
        virtual ~VulkanFramebuffer() override;

        virtual const FramebufferSpecification& GetSpecification() const override { return mSpecification; }
        virtual FramebufferSpecification& GetSpecification() override { return mSpecification; }
        virtual const Ref<Image2D>& GetColorAttachment(Uint index) const override { return mColorAttachmentImages[index]; }

        // TODO: Maybe move to separate class?
        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmd) const override;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmd) const override;

        VkFramebuffer& GetVulkanFramebuffer() { return mFramebuffer; }
        VkRenderPass& GetVulkanRenderPass() { return mRenderPass; }

    private:
        void Invalidate();

    private:
        FramebufferSpecification mSpecification;
        Vector<Ref<Image2D>> mColorAttachmentImages;
        Ref<Image2D> mDepthAttachmentImage;

        VkRenderPass mRenderPass = VK_NULL_HANDLE;
        VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    };
} // namespace Surge
