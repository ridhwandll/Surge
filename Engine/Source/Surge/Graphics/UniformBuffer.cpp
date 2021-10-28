// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/UniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"

namespace Surge
{
    Ref<UniformBuffer> UniformBuffer::Create(Uint size)
    {
        return Ref<VulkanUniformBuffer>::Create(size);
    }

} // namespace Surge
