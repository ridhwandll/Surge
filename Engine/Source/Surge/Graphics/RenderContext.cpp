// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"

namespace Surge
{
    Scope<RenderContext> RenderContext::Create()
    {
        return CreateScope<VulkanRenderContext>();
    }
}
