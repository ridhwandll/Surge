// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    Ref<RenderCommandBuffer> RenderCommandBuffer::Create(bool createFromSwapchain, Uint size, const String& debugName)
    {
        return Ref<VulkanRenderCommandBuffer>::Create(createFromSwapchain, size, debugName);
    }
} // namespace Surge
