// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/RenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include <volk.h>

namespace Surge
{
    class SURGE_API VulkanRenderContext : public RenderContext
    {
    public:
        virtual void Initialize(Window* window) override;
        virtual void Shutdown() override;

        virtual void OnResize(Uint width, Uint height) override;
        virtual void* GetInteralDevice() override { return &mDevice; }
        virtual void* GetInteralInstance() override { return mVulkanInstance; }
    private:
        Vector<const char*> GetRequiredInstanceExtensions();
        Vector<const char*> GetRequiredInstanceLayers();
    private:
        VkInstance mVulkanInstance;
        VulkanDiagnostics mVulkanDiagnostics;
        VulkanDevice mDevice;
        VulkanSwapChain mSwapChain;
    };
}
