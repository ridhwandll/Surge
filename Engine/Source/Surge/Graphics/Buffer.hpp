// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    enum class BufferType
    {
        None = -1,
        VertexBuffer = 0,
        IndexBuffer
    };

    class Buffer : public RefCounted
    {
    public:
        Buffer() = default;
        virtual ~Buffer() {}

        virtual Uint GetSize() = 0;

        static Ref<Buffer> Create(const void* data, const Uint& size, const BufferType& type);
    };
}
