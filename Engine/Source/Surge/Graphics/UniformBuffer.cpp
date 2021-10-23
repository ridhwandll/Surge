// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/UniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"

namespace Surge
{
    Ref<UniformBuffer> UniformBuffer::Create(Uint size, Uint binding)
    {
        return Ref<VulkanUniformBuffer>::Create(size, binding);
    }

} // namespace Surge
