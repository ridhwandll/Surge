// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Buffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanBuffer.hpp"

namespace Surge
{
    Ref<Buffer> Buffer::Create(const void* data, const Uint& size, const BufferType& type)
    {
        return Ref<VulkanBuffer>::Create(data, size, type);
    }
}
