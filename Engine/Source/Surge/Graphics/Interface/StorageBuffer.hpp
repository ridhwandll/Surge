// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Core/Buffer.hpp"

namespace Surge
{
    class SURGE_API StorageBuffer : public RefCounted
    {
    public:
        virtual ~StorageBuffer() = default;

        virtual void SetData(const void* data, Uint offset = 0) const = 0;
        virtual void SetData(const Buffer& data, Uint offset = 0) const = 0;
        virtual Uint GetSize() const = 0;
        virtual void Resize(Uint newSize) = 0;

        static Ref<StorageBuffer> Create(Uint size, GPUMemoryUsage memoryUsage);
    };

} // namespace Surge
