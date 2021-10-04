// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"

namespace Surge
{
    class VertexBuffer : public RefCounted
    {
    public:
        VertexBuffer() = default;
        virtual ~VertexBuffer() = default;

        virtual Uint GetSize() = 0;
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const = 0;

        static Ref<VertexBuffer> Create(const void* data, const Uint& size);
    };
} // namespace Surge
