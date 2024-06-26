// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Interface/RenderCommandBuffer.hpp"
#include <volk.h>

namespace Surge
{
    class SURGE_API VulkanRenderCommandBuffer : public RenderCommandBuffer
    {
    public:
        VulkanRenderCommandBuffer(bool createFromSwapchain, Uint size = 0);
        virtual ~VulkanRenderCommandBuffer() override;

        virtual void BeginRecording() override;
        virtual void EndRecording() override;
        virtual void Submit() override;

        VkCommandPool GetVulkanCommandPool() const { return mCommandPool; }
        VkCommandBuffer GetVulkanCommandBuffer(Uint index) const { return mCommandBuffers[index]; }

    private:
        bool mCreatedFromSwapchain;
        VkCommandPool mCommandPool {};
        Vector<VkCommandBuffer> mCommandBuffers {};

        // Sync Objects
        Vector<VkFence> mWaitFences {};
    };
} // namespace Surge
