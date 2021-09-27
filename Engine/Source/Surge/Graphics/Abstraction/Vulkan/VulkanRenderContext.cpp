// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"

namespace Surge
{
#ifdef SURGE_DEBUG
    #define ENABLE_IF_VK_VALIDATION(x) { x; }
#else
    #define ENABLE_IF_VK_VALIDATION(x)
#endif // SURGE_DEBUG

    void VulkanRenderContext::Initialize(Window* window)
    {
        VK_CALL(volkInitialize());

        /// VkApplicationInfo ///
        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = "SurgeProtector";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Surge Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2; // TODO(Rid): Check which version is available, use 1.1 if necessary

        /// VkInstanceCreateInfo ///
        Vector<const char*> instanceExtensions = GetRequiredInstanceExtensions();
        Vector<const char*> instanceLayers = GetRequiredInstanceLayers();

        VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<Uint>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();
        createInfo.enabledExtensionCount = static_cast<Uint>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        createInfo.pNext = nullptr;

        ENABLE_IF_VK_VALIDATION(mVulkanDiagnostics.Create(createInfo));
        VK_CALL(vkCreateInstance(&createInfo, nullptr, &mVulkanInstance));
        ENABLE_IF_VK_VALIDATION(mVulkanDiagnostics.StartDiagnostics(mVulkanInstance));
        volkLoadInstance(mVulkanInstance);

        mDevice.Initialize(mVulkanInstance);
        mSwapChain.Initialize(window);
        mMemoryAllocator.Initialize(mVulkanInstance, mDevice);
        mImGuiContext.Initialize(this);

        //Fill In GPUInfo
        mGPUInfo.Name = mDevice.GetProperties().vk10Properties.properties.deviceName;
        mGPUInfo.DeviceScore = mDevice.GetDeviceScore();
    }

    void VulkanRenderContext::BeginFrame()
    {
        mSwapChain.BeginFrame();
        mImGuiContext.BeginFrame();
    }

    void VulkanRenderContext::EndFrame()
    {
        mImGuiContext.EndFrame();
        mSwapChain.EndFrame();
    }

    void VulkanRenderContext::Shutdown()
    {
        mImGuiContext.Destroy();
        mMemoryAllocator.Destroy();
        mSwapChain.Destroy();
        ENABLE_IF_VK_VALIDATION(mVulkanDiagnostics.EndDiagnostics(mVulkanInstance));
        mDevice.Destroy();
        vkDestroyInstance(mVulkanInstance, nullptr);
    }

    void VulkanRenderContext::OnResize()
    {
        mSwapChain.Resize();
    }

    void VulkanRenderContext::RenderImGui()
    {
        mImGuiContext.Render();
    }

    Vector<const char*> VulkanRenderContext::GetRequiredInstanceExtensions()
    {
        Vector<const char*> instanceExtensions;
        instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME); // Currently windows Only
        ENABLE_IF_VK_VALIDATION(mVulkanDiagnostics.AddValidationExtensions(instanceExtensions));
        return instanceExtensions;
    }

    Vector<const char*> VulkanRenderContext::GetRequiredInstanceLayers()
    {
        Vector<const char*> instanceLayers;
        ENABLE_IF_VK_VALIDATION(mVulkanDiagnostics.AddValidationLayers(instanceLayers));
        return instanceLayers;
    }
}
