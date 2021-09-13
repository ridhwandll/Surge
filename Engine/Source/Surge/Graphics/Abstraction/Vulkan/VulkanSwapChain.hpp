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
        void BeginFrame();
        void EndFrame();
        void Resize();
        void Present();
        void Destroy();

        Uint GetImageCount() const { return mImageCount; }
        Uint GetWidth() const { return mSwapChainExtent.width; }
        Uint GetHeight() const { return mSwapChainExtent.height; }

        VkExtent2D GetVulkanExtent2D() const { return mSwapChainExtent; }
        VkFormat GetVulkanColorFormat() const { return mColorFormat.format; }
        VkSwapchainKHR GetVulkanSwapChain() const { return mSwapChain; }
        VkRenderPass GetVulkanRenderPass() const { return mRenderPass; }
        VkFramebuffer GetVulkanFramebuffer() const { return mFramebuffer; }
        VkCommandPool GetVulkanCommandPool() const { return mCommandPool; }
        Vector<VkImageView> GetVulkanImageViews() const { return mSwapChainImageViews; }
        Vector<VkCommandBuffer> GetVulkanCommandBuffers() const { return mCommandBuffers; }
        Uint GetCurrentFrameIndex() { return mCurrentFrameIndex; }
    private:
        VkResult AcquireNextImage(VkSemaphore imageAvailableSemaphore, Uint* imageIndex);
        void PickPresentQueue();
        void CreateSwapChain();
        void CreateRenderPass();
        void CreateFramebuffer();
        void CreateCmdBuffers();
        void CreateSyncObjects();
    private:
        // Swapchain
        VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
        VkSurfaceKHR mSurface;
        VkExtent2D mSwapChainExtent;
        VkSurfaceFormatKHR mColorFormat;
        Uint mImageCount;
        Uint mCurrentImageIndex = 0;
        Uint mCurrentFrameIndex = 0;

        // Framebuffers + Renderpasses
        VkRenderPass mRenderPass;
        Vector<VkImage> mSwapChainImages;
        Vector<VkImageView> mSwapChainImageViews;
        VkFramebuffer mFramebuffer;

        // Presentation Queue stuff
        VkQueue mPresentQueue;
        Uint mPresentQueueIndex;

        // Commandbuffer
        VkCommandPool mCommandPool = VK_NULL_HANDLE;
        Vector<VkCommandBuffer> mCommandBuffers{};

        // Sync objects
        VkSemaphore mImageAvailable = VK_NULL_HANDLE, mRenderAvailable = VK_NULL_HANDLE;
        Vector<VkFence> mWaitFences{};

        bool mVsync = false;
    };
}
