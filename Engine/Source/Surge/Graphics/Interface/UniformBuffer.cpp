// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/UniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"

namespace Surge
{
    Ref<UniformBuffer> UniformBuffer::Create(Uint size)
    {
        return Ref<VulkanUniformBuffer>::Create(size);
    }

} // namespace Surge
