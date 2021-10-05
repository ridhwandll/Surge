// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanImGuiContext
    {
    public:
        VulkanImGuiContext() = default;
        ~VulkanImGuiContext() = default;

    private:
        void Initialize(void* vulkanRenderContext);
        void Destroy();

        void BeginFrame();
        void Render(); // Must be called inside a "Graphics" command buffer
        void EndFrame();

        void* AddImage(const Ref<Image2D>& image2d) const;
        void SetDarkThemeColors();

    private:
        VkDescriptorPool mImguiPool;
        void* mVulkanRenderContext = nullptr;

    private:
        friend class VulkanRenderContext;
    };

} // namespace Surge
