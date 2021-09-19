// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImGuiContext.hpp"
#include <volk.h>

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

        virtual void* GetInternalDevice() override { return &mDevice; }
        virtual void* GetInternalInstance() override { return mVulkanInstance; }
        virtual void* GetSwapChain() override { return &mSwapChain; }

        virtual GPUMemoryStats GetMemoryStatus() const override { return mMemoryAllocator.GetStats(); };
        virtual void* GetMemoryAllocator() const override { return (void*)&mMemoryAllocator; }
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

        friend class VulkanImGuiContext;
        friend class VulkanDevice;
    };
}
