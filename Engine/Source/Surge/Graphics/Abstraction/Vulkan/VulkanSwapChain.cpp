// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"

namespace Surge
{
    void VulkanSwapChain::Initialize(Window* window)
    {
        VkInstance instance = static_cast<VkInstance>(CoreGetRenderContext()->GetInteralInstance());
        VulkanUtils::CreateWindowSurface(instance, window, &mSurface);
        PickPresentQueue();
        CreateSwapChain();
        CreateRenderPass();
        CreateFramebuffer();
    }

    void VulkanSwapChain::CreateSwapChain()
    {
        VkPhysicalDevice physicalDevice = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetSelectedPhysicalDevice();
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

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
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_CALL(vkCreateSwapchainKHR(device, &createInfo, nullptr, &mSwapChain));

        vkGetSwapchainImagesKHR(device, mSwapChain, &imageCount, nullptr);
        mSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, mSwapChain, &imageCount, mSwapChainImages.data());

        // Setting up the members
        mColorFormat = pickedFormat;
        mSwapChainExtent = swapChainExtent;
    }

    void VulkanSwapChain::CreateRenderPass()
    {
        // Creating here the renderPass because we need it for the framebuffer, for imgui and other stuff.
        // The rendered image will be passed to this renderpass by rendering a quad with the final texture or viewport and then it will pe presented
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

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

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

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
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.pNext = &attachmentCreateInfo;
        framebufferInfo.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = nullptr;
        framebufferInfo.width = mSwapChainExtent.width;
        framebufferInfo.height = mSwapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_CALL(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffer));
    }

    void VulkanSwapChain::Resize(Uint width, Uint height)
    {
        VkInstance instance = static_cast<VkInstance>(CoreGetRenderContext()->GetInteralInstance());
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

        // Wait till everything has finished rendering before deleting it
        vkDeviceWaitIdle(device);

        vkDestroyFramebuffer(device, mFramebuffer, nullptr);
        for (auto& imageView : mSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        vkDestroySwapchainKHR(device, mSwapChain, nullptr);

        CreateSwapChain();
        CreateFramebuffer();
    }

    void VulkanSwapChain::Destroy()
    {
        VkInstance instance = static_cast<VkInstance>(CoreGetRenderContext()->GetInteralInstance());
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

        vkDestroyRenderPass(device, mRenderPass, nullptr);
        vkDestroyFramebuffer(device, mFramebuffer, nullptr);

        for (auto& imageView : mSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        vkDestroySwapchainKHR(device, mSwapChain, nullptr);
        vkDestroySurfaceKHR(instance, mSurface, nullptr);
    }

    VkResult VulkanSwapChain::AcquireNextImage(VkSemaphore imageAvailableSemaphore, Uint* imageIndex)
    {
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();
        VkResult result = vkAcquireNextImageKHR(device, mSwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, imageIndex);
        VK_CALL(result);
        return result;
    }

    VkResult VulkanSwapChain::Present(Uint imageIndex, VkSemaphore waitSempahore)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSempahore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);
        return result;
    }

    void VulkanSwapChain::PickPresentQueue()
    {
        VkPhysicalDevice physicalDevice = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetSelectedPhysicalDevice();
        VkDevice device = static_cast<VulkanDevice*>(CoreGetRenderContext()->GetInteralDevice())->GetLogicaldevice();

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
