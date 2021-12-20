// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Interface/UniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanStorageBuffer.hpp"

namespace Surge
{
    Ref<StorageBuffer> StorageBuffer::Create(Uint size, GPUMemoryUsage memoryUsage)
    {
        return Ref<VulkanStorageBuffer>::Create(size, memoryUsage);
    }

} // namespace Surge
