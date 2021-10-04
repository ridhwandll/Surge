// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanFramebuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
        : mSpecification(spec)
    {
        Invalidate();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();

        if (mRenderPass)
            vkDestroyRenderPass(logicalDevice, mRenderPass, nullptr);
        if (mFramebuffer)
            vkDestroyFramebuffer(logicalDevice, mFramebuffer, nullptr);
    }

    void VulkanFramebuffer::BeginRenderPass(const Ref<RenderCommandBuffer>& cmd) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = cmd.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {mSpecification.ClearColor.r, mSpecification.ClearColor.g, mSpecification.ClearColor.b, mSpecification.ClearColor.a};
        clearValues[1].depthStencil = {1.0f, 0};

        VkViewport viewport = {};
        viewport.width = float(mSpecification.Width);
        viewport.height = float(mSpecification.Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.extent = {mSpecification.Width, mSpecification.Height};
        scissor.offset = {0, 0};

        VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = mRenderPass;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {mSpecification.Width, mSpecification.Height};
        renderPassBeginInfo.framebuffer = mFramebuffer;
        renderPassBeginInfo.clearValueCount = Uint(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdSetViewport(vulkanCmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(vulkanCmdBuffer, 0, 1, &scissor);
        vkCmdBeginRenderPass(vulkanCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanFramebuffer::EndRenderPass(const Ref<RenderCommandBuffer>& cmd) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        vkCmdEndRenderPass(cmd.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex));
    }

    void VulkanFramebuffer::Invalidate()
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();

        Vector<VkAttachmentDescription> attachmentDescriptions;

        Vector<VkAttachmentReference> colorAttachmentReferences;
        VkAttachmentReference depthAttachmentReference;

        Uint attachmentIndex = 0;
        for (ImageFormat& format : mSpecification.Formats)
        {
            ImageSpecification imageSpec;
            imageSpec.Format = format;
            imageSpec.Width = mSpecification.Width;
            imageSpec.Height = mSpecification.Height;
            imageSpec.Usage = ImageUsage::Attachment;
            imageSpec.Mips = 1;
            Ref<Image2D> image = Image2D::Create(imageSpec);

            if (VulkanUtils::IsDepthFormat(format))
            {
                //TODO: Depth Attachment
                mDepthAttachmentImage = image;
            }
            else
            {
                mColorAttachmentImages.push_back(image);
                VkAttachmentDescription& colorAttachment = attachmentDescriptions.emplace_back();
                colorAttachment.format = VulkanUtils::GetImageFormat(format);
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // For now, we don't care about stencil
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // ^^
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // We don't care what previous layout the image was in
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkAttachmentReference colorAttachmentReference {};
                colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentReference.attachment = attachmentIndex; // Specifies which attachment to reference by it's index in the attachment descriptions array
                colorAttachmentReferences.emplace_back(colorAttachmentReference);
            }
            attachmentIndex++;
        }

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = Uint(colorAttachmentReferences.size());
        subpassDescription.pColorAttachments = colorAttachmentReferences.data();
        //TODO: Depth Attachment

        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // Create the Framebuffer
        VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassInfo.attachmentCount = Uint(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        VK_CALL(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &mRenderPass));

        Uint colorAttachmentImagesSize = mColorAttachmentImages.size();
        Vector<VkImageView> attachments(colorAttachmentImagesSize);
        for (Uint i = 0; i < colorAttachmentImagesSize; i++)
        {
            Ref<VulkanImage2D> image = mColorAttachmentImages[i].As<VulkanImage2D>();
            attachments[i] = image->GetVulkanImageView();
            SG_ASSERT(attachments[i], "Invalid Vulkan ImageView!");
        }
        //TODO: Depth Attachment

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = mRenderPass;
        framebufferCreateInfo.attachmentCount = uint32_t(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = mSpecification.Width;
        framebufferCreateInfo.height = mSpecification.Height;
        framebufferCreateInfo.layers = 1;
        VK_CALL(vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &mFramebuffer));
    }

} // namespace Surge
