// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/UniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanStorageBuffer.hpp"

namespace Surge
{
    Ref<StorageBuffer> StorageBuffer::Create(Uint size)
    {
        return Ref<VulkanStorageBuffer>::Create(size);
    }

} // namespace Surge
