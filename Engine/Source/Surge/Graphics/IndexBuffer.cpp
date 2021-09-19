// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanIndexBuffer.hpp"

namespace Surge
{
    Ref<IndexBuffer> IndexBuffer::Create(const void* data, const Uint& size)
    {
        return Ref<VulkanIndexBuffer>::Create(data, size);
    }
}