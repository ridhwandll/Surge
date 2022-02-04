// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    Ref<RenderCommandBuffer> RenderCommandBuffer::Create(bool createFromSwapchain, Uint size)
    {
        return Ref<VulkanRenderCommandBuffer>::Create(createFromSwapchain, size);
    }
} // namespace Surge
