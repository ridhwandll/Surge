// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImGuiContext.hpp"
#include <volk.h>

#define SURGE_GET_VULKAN_CONTEXT(renderContext) renderContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get())

namespace Surge
{
    class VulkanRenderContext : public RenderContext
    {
    public:
        virtual void Initialize(Window* window) override;
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void Shutdown() override;
        virtual void OnResize() override;
        virtual void RenderImGui() override;

        Uint GetFrameIndex() const override { return mSwapChain.GetCurrentFrameIndex(); }
        virtual GPUMemoryStats GetMemoryStatus() const override { return mMemoryAllocator.GetStats(); };
        virtual GPUInfo GetGPUInfo() const override { return mGPUInfo; }
        virtual void* GetMemoryAllocator() const override { return (void*)&mMemoryAllocator; }

        VkInstance GetInstance() const { return mVulkanInstance; }
        VulkanDevice* GetDevice() { return &mDevice; }
        VulkanSwapChain* GetSwapChain() { return &mSwapChain; }
    private:
        Vector<const char*> GetRequiredInstanceExtensions();
        Vector<const char*> GetRequiredInstanceLayers();
    private:
        VkInstance mVulkanInstance = VK_NULL_HANDLE;
        VulkanDiagnostics mVulkanDiagnostics{};
        VulkanDevice mDevice{};
        VulkanSwapChain mSwapChain{};
        VulkanMemoryAllocator mMemoryAllocator{};
        VulkanImGuiContext mImGuiContext;

        GPUInfo mGPUInfo;
        friend class VulkanImGuiContext;
        friend class VulkanDevice;
    };
}
