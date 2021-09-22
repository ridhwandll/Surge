// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanRenderCommandBuffer : public RenderCommandBuffer
    {
    public:
        VulkanRenderCommandBuffer(bool createFromSwapchain, Uint size = 0, const String& debugName = "");
        virtual ~VulkanRenderCommandBuffer() override;

        virtual void BeginRecording() override;
        virtual void EndRecording() override;
        virtual void Submit() override;

        VkCommandPool GetVulkanCommandPool() const { mCommandPool; }
        VkCommandBuffer GetVulkanCommandBuffer(Uint index) const { return mCommandBuffers[index]; }
    private:
        bool mCreatedFromSwapchain;
        VkCommandPool mCommandPool{};
        Vector<VkCommandBuffer> mCommandBuffers{};

        // Sync Objects
        Vector<VkFence> mWaitFences{};
    };
}
