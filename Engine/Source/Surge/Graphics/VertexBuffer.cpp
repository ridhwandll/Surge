// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanVertexBuffer.hpp"

namespace Surge
{
    Ref<VertexBuffer> VertexBuffer::Create(const void* data, const Uint& size) { return Ref<VulkanVertexBuffer>::Create(data, size); }
} // namespace Surge
