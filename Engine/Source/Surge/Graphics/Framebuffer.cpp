// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Framebuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanFramebuffer.hpp"

namespace Surge
{
    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        return Ref<VulkanFramebuffer>::Create(spec);
    }

} // namespace Surge