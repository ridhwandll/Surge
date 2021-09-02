// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanRenderContext : public RenderContext
    {
    public:
        virtual void Initialize(Window* window) override;
        virtual void Shutdown() override;

        virtual void OnResize(Uint width, Uint height) override;

        virtual void* GetInteralDevice() override { return &mDevice; }
        virtual void* GetInteralInstance() override { return mVulkanInstance; }

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
    };
}
