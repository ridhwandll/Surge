// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"

namespace Surge
{
    Scope<RenderContext> RenderContext::Create()
    {
        return CreateScope<VulkanRenderContext>();
    }
}
