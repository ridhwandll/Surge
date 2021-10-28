// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImGuiContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMemoryAllocator.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include <volk.h>

#define SURGE_GET_VULKAN_CONTEXT(renderContext) renderContext = static_cast<::Surge::VulkanRenderContext*>(::Surge::SurgeCore::GetRenderContext())

namespace Surge
{
    class VulkanRenderContext : public RenderContext
    {
    public:
        virtual void Initialize(Window* window, bool enableImGui = true) override;
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void Shutdown() override;
        virtual void OnResize() override;
        virtual void RenderImGui() override;

        Uint GetFrameIndex() const override { return mSwapChain.GetCurrentFrameIndex(); }
        virtual GPUMemoryStats GetMemoryStatus() const override { return mMemoryAllocator.GetStats(); };
        virtual GPUInfo GetGPUInfo() const override { return mGPUInfo; }

        VkInstance GetInstance() const { return mVulkanInstance; }
        VulkanDevice* GetDevice() { return &mDevice; }
        VulkanSwapChain* GetSwapChain() { return &mSwapChain; }
        VulkanMemoryAllocator* GetMemoryAllocator() { return &mMemoryAllocator; }

        virtual void* GetImGuiTextureID(const Ref<Image2D>& image) const;

    private:
        Vector<const char*> GetRequiredInstanceExtensions();
        Vector<const char*> GetRequiredInstanceLayers();

    private:
        VkInstance mVulkanInstance = VK_NULL_HANDLE;
        VulkanDiagnostics mVulkanDiagnostics {};
        VulkanDevice mDevice {};
        VulkanSwapChain mSwapChain {};
        VulkanMemoryAllocator mMemoryAllocator {};
        VulkanImGuiContext mImGuiContext;
        bool mImGuiEnabled;

        GPUInfo mGPUInfo;
        friend class VulkanImGuiContext;
        friend class VulkanDevice;
    };

    template <typename T>
    FORCEINLINE void SetDebugVkResourceName(T handle, VkObjectType type, const char* name)
    {
        static_assert(sizeof(T) == sizeof(uint64_t), "Invalid handle");
        VkDebugUtilsObjectNameInfoEXT info {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        info.objectType = type;
        info.objectHandle = reinterpret_cast<uint64_t>(handle);
        info.pObjectName = name;

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        vkSetDebugUtilsObjectNameEXT(renderContext->GetDevice()->GetLogicalDevice(), &info);
    }

#if SURGE_DEBUG
#define SET_VK_OBJECT_DEBUGNAME(handle, type, name) SetDebugVkResourceName(handle, type, fmt::format("{0}:{1} - {2}", __FILE__, __LINE__, name).c_str())
#else
#define SET_VK_OBJECT_DEBUGNAME(handle, type, name)
#endif

} // namespace Surge
