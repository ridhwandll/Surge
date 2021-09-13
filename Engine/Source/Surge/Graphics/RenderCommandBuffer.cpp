// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    Ref<RenderCommandBuffer> RenderCommandBuffer::Create(Uint size, bool createFromSwapchain, const String& debugName)
    {
        return Ref<VulkanRenderCommandBuffer>::Create(size, createFromSwapchain, debugName);
    }
}
