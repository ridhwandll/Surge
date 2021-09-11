// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanRenderCommandBuffer : public RenderCommandBuffer
    {
    public:
        VulkanRenderCommandBuffer(Uint size = 0, const String& debugName = "");
        virtual ~VulkanRenderCommandBuffer() override;

        virtual void BeginRecording() override;
        virtual void EndRecording() override;
        virtual void Submit() override;

        VkCommandPool GetVulkanCommandPool() const { mCommandPool; }
        VkCommandBuffer GetVulkanCommandBuffer(Uint index) const { return mCommandBuffers[index]; }
    private:
        VkCommandPool mCommandPool{};
        Vector<VkCommandBuffer> mCommandBuffers{};

        // Sync Objects
        Vector<VkFence> mWaitFences{};
    };
}
