// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    enum class BufferType
    {
        None = -1,
        VertexBuffer = 0,
        IndexBuffer,
        UniformBuffer
    };

    class Buffer : public RefCounted
    {
    public:
        Buffer() = default;
        virtual ~Buffer() = default;

        virtual Uint GetSize() = 0;
        virtual void SetData(const void* data, const Uint& size) = 0;

        static Ref<Buffer> Create(const void* data, const Uint& size, const BufferType& type);
    };
}
