// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"

#include <array>

namespace Surge
{
    void VulkanSwapChain::Initialize(Window* window)
    {
        VkInstance instance = static_cast<VkInstance>(CoreGetRenderContext()->GetInternalInstance());
        VulkanUtils::CreateWindowSurface(instance, window, &mSurface);
        PickPresentQueue();
        CreateSwapChain();
        CreateRenderPass();
        CreateFramebuffer();
        CreateCmdBuffers();
        CreateSyncObjects();
    }

    void VulkanSwapChain::CreateSwapChain()
    {
        VkPhysicalDevice physicalDevice = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetPhysicaldevice();
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface, &surfaceCapabilities);

        Uint imageCount = surfaceCapabilities.minImageCount + 1;

        VkExtent2D swapChainExtent = {};
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            swapChainExtent = surfaceCapabilities.currentExtent;

        // Getting all swapchain formats
        Uint availableFormatCount = 0;
        Vector<VkSurfaceFormatKHR> availableFormats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &availableFormatCount, nullptr);
        availableFormats.resize(availableFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &availableFormatCount, availableFormats.data());

        // Selecting the best swapchain format
        VkSurfaceFormatKHR pickedFormat = availableFormats[0]; // Default one is `availableFormats[0]`
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                pickedFormat = availableFormat;
        }

        // Getting all swapchain presentModes
        Uint availablePresentModeCount = 0;
        Vector<VkPresentModeKHR> avaialbePresentModes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface, &availablePresentModeCount, nullptr);
        avaialbePresentModes.resize(availablePresentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface, &availablePresentModeCount, avaialbePresentModes.data());

        // Selecting the best swapchain present mode
        VkPresentModeKHR pickedPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Default one
        for (const auto& availablePresentMode : avaialbePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR && !mVsync)
                pickedPresentMode = availablePresentMode;
        }

        VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = mSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = pickedFormat.format;
        createInfo.imageColorSpace = pickedFormat.colorSpace;
        createInfo.imageExtent = swapChainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO(AC3R): Shoud add support for `VK_SHARING_MODE_CONCURRENT` later
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = pickedPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = mSwapChain ? mSwapChain : VK_NULL_HANDLE;

        VK_CALL(vkCreateSwapchainKHR(device, &createInfo, nullptr, &mSwapChain));

        vkGetSwapchainImagesKHR(device, mSwapChain, &imageCount, nullptr);
        mSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, mSwapChain, &imageCount, mSwapChainImages.data());

        // Setting up the members
        mColorFormat = pickedFormat;
        mSwapChainExtent = swapChainExtent;
        mImageCount = imageCount;
    }

    void VulkanSwapChain::CreateRenderPass()
    {
        // Creating here the renderPass because we need it for the framebuffer, for imgui and other stuff.
        // The rendered image will be passed to this renderpass by rendering a quad with the final texture or viewport and then it will pe presented
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = mColorFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VK_CALL(vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass));
    }

    void VulkanSwapChain::CreateFramebuffer()
    {
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();

        // Creating the VkImageViews
        mSwapChainImageViews.resize(mSwapChainImages.size());
        for (Uint i = 0; i < mSwapChainImages.size(); i++)
        {
            VkImage swapChainImage = mSwapChainImages[i];

            // Specifying that we are gonna use the imageviews as color attachments
            VkImageViewUsageCreateInfo usageInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
            usageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            createInfo.pNext = &usageInfo; // using pNext chain for letting vulkan know that this imageView is gonna be used for the imageless framebuffer
            createInfo.image = swapChainImage;
            createInfo.format = mColorFormat.format;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            VK_CALL(vkCreateImageView(device, &createInfo, nullptr, &mSwapChainImageViews[i]));
        }

        // Imageless framebuffer (Vulkan Core feature 1.2)
        // Firstly we tell vulkan how we are gonna use the image views
        VkFramebufferAttachmentImageInfo attachmentImageInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO };
        attachmentImageInfo.pNext = nullptr;
        attachmentImageInfo.width = mSwapChainExtent.width;
        attachmentImageInfo.height = mSwapChainExtent.height;
        attachmentImageInfo.flags = 0;
        attachmentImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        attachmentImageInfo.layerCount = 1;
        attachmentImageInfo.viewFormatCount = 1;
        attachmentImageInfo.pViewFormats = &mColorFormat.format;

        // Reference the struct above here and specify how many attachments we have
        VkFramebufferAttachmentsCreateInfo attachmentCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO };
        attachmentCreateInfo.attachmentImageInfoCount = 1;
        attachmentCreateInfo.pAttachmentImageInfos = &attachmentImageInfo;

        // Creation of the framebuffer is done using the pNext chain and with the flag `VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT`
        VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.pNext = &attachmentCreateInfo;
        framebufferInfo.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = nullptr; // Yes, nullptr
        framebufferInfo.width = mSwapChainExtent.width;
        framebufferInfo.height = mSwapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_CALL(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffer));
    }

    void VulkanSwapChain::CreateCmdBuffers()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanDevice* device = static_cast<VulkanDevice*>(vkContext->GetInternalDevice());
        VkDevice logicalDevice = device->GetLogicaldevice();

        // Command Pool Creation
        VkCommandPoolCreateInfo cmdPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cmdPoolInfo.queueFamilyIndex = device->GetQueueFamilyIndices().GraphicsQueue;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CALL(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &mCommandPool));

        // Command Buffers
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        commandBufferAllocateInfo.commandPool = mCommandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = mImageCount;
        mCommandBuffers.resize(mImageCount);
        VK_CALL(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, mCommandBuffers.data()));
    }

    void VulkanSwapChain::CreateSyncObjects()
    {
        Uint framesInFlight = FRAMES_IN_FLIGHT; //TODO: More than two Frames in Flight
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = static_cast<VulkanDevice*>(vkContext->GetInternalDevice())->GetLogicaldevice();

        // Semaphores
        VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CALL(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &mImageAvailable));
        VK_CALL(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &mRenderAvailable));

        // Fences
        VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        mWaitFences.resize(framesInFlight);
        for (VkFence& fence : mWaitFences)
            VK_CALL(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }

    void VulkanSwapChain::Resize()
    {
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();
        mCurrentFrameIndex = 0;

        // Wait till everything has finished rendering before deleting it
        vkDeviceWaitIdle(device);

        vkDestroyFramebuffer(device, mFramebuffer, nullptr);
        for (auto& imageView : mSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        CreateSwapChain();
        CreateFramebuffer();
    }

    void VulkanSwapChain::Destroy()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkInstance instance = static_cast<VkInstance>(vkContext->GetInternalInstance());
        VkDevice device = static_cast<VulkanDevice*>(vkContext->GetInternalDevice())->GetLogicaldevice();

        vkDeviceWaitIdle(device);

        vkDestroyRenderPass(device, mRenderPass, nullptr);
        vkDestroyFramebuffer(device, mFramebuffer, nullptr);

        for (auto& imageView : mSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        vkDestroySwapchainKHR(device, mSwapChain, nullptr);
        vkDestroySurfaceKHR(instance, mSurface, nullptr);

        vkDestroyCommandPool(device, mCommandPool, nullptr);
        vkDestroySemaphore(device, mImageAvailable, nullptr);
        vkDestroySemaphore(device, mRenderAvailable, nullptr);
        for (VkFence& fence : mWaitFences)
            vkDestroyFence(device, fence, nullptr);
    }

    VkResult VulkanSwapChain::AcquireNextImage(VkSemaphore imageAvailableSemaphore, Uint* imageIndex)
    {
        // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();
        return vkAcquireNextImageKHR(device, mSwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, imageIndex);
    }

    void VulkanSwapChain::BeginFrame()
    {
        VulkanDevice* device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice());
        VK_CALL(vkWaitForFences(device->GetLogicaldevice(), 1, &mWaitFences[mCurrentFrameIndex], VK_TRUE, UINT64_MAX));
        VK_CALL(AcquireNextImage(mImageAvailable, &mCurrentImageIndex));
    }

    void VulkanSwapChain::Present()
    {
        VulkanDevice* device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice());

        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.pWaitSemaphores = &mImageAvailable;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &mRenderAvailable;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrameIndex];
        submitInfo.commandBufferCount = 1;

        VK_CALL(vkResetFences(device->GetLogicaldevice(), 1, &mWaitFences[mCurrentFrameIndex]));
        VK_CALL(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, mWaitFences[mCurrentFrameIndex]));

        // Present
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderAvailable;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.pImageIndices = &mCurrentImageIndex;
        VK_CALL(vkQueuePresentKHR(mPresentQueue, &presentInfo));

        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
    }

    void VulkanSwapChain::EndFrame()
    {
        Present();
    }

    void VulkanSwapChain::BeginRenderPass()
    {
        VkRenderPassAttachmentBeginInfo attachmentInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO };
        attachmentInfo.attachmentCount = 1;
        attachmentInfo.pAttachments = &mSwapChainImageViews[mCurrentImageIndex];

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = mRenderPass;
        renderPassInfo.framebuffer = mFramebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = mSwapChainExtent;
        renderPassInfo.pNext = &attachmentInfo; // Imageless framebuffer
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(mCommandBuffers[mCurrentFrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanSwapChain::EndRenderPass()
    {
        vkCmdEndRenderPass(mCommandBuffers[mCurrentFrameIndex]);
    }

    void VulkanSwapChain::PickPresentQueue()
    {
        VkPhysicalDevice physicalDevice = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetPhysicaldevice();
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInternalDevice())->GetLogicaldevice();

        // Getting all the queueFamilies
        Vector<VkQueueFamilyProperties> queueFamilyProps;
        Uint queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        queueFamilyProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

        // Picking the one that has presentQueue
        for (Uint i = 0; i < queueFamilyProps.size(); i++)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, mSurface, &presentSupport);

            if (presentSupport)
            {
                mPresentQueueIndex = i;
                break;
            }
        }

        // Getting the VkQueue using the presentQueueIndex
        vkGetDeviceQueue(device, mPresentQueueIndex, 0, &mPresentQueue);
    }
}
