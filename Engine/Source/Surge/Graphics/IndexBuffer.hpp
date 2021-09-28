// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"

namespace Surge
{
    class IndexBuffer : public RefCounted
    {
    public:
        IndexBuffer() = default;
        virtual ~IndexBuffer() = default;

        virtual Uint GetSize() const = 0;
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const = 0;

        static Ref<IndexBuffer> Create(const void* data, const Uint& size);
    };
}
