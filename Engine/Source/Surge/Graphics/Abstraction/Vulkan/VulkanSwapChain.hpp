// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanSwapChain
    {
    public:
        VulkanSwapChain() = default;
        ~VulkanSwapChain() = default;

        void Initialize(Window* window);
        void Resize(Uint width, Uint height);
        void Destroy();

        Uint GetImageCount() const { return mImageCount; }
        Uint GetWidth() const { return mSwapChainExtent.width; }
        Uint GetHeight() const { return mSwapChainExtent.height; }
        VkExtent2D GetExtent() const { return mSwapChainExtent; }
        VkFormat GetColorFormat() const { return mColorFormat.format; }
        VkSwapchainKHR GetVulkanSwapChain() const { return mSwapChain; }
        VkRenderPass GetVulkanRenderPass() const { return mRenderPass; }
        VkFramebuffer GetFramebuffer() const { return mFramebuffer; }
        Vector<VkImageView> GetImageViews() const { return mSwapChainImageViews; }
    private:
        VkResult AcquireNextImage(VkSemaphore imageAvailableSemaphore, Uint* imageIndex);
        VkResult Present(Uint imageIndex, VkSemaphore waitSempahore);
        void PickPresentQueue();
        void CreateSwapChain();
        void CreateRenderPass();
        void CreateFramebuffer();
    private:
        VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
        VkSurfaceKHR mSurface;

        VkRenderPass mRenderPass;
        Vector<VkImage> mSwapChainImages;
        Vector<VkImageView> mSwapChainImageViews;
        VkFramebuffer mFramebuffer;

        Uint mImageCount;
        VkExtent2D mSwapChainExtent;
        VkSurfaceFormatKHR mColorFormat;

        // Presentation Queue stuff
        VkQueue mPresentQueue;
        Uint mPresentQueueIndex;

        bool mVsync = false;
    };
}
