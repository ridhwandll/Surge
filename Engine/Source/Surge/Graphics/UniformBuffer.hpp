// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class UniformBuffer : public RefCounted
    {
    public:
        virtual ~UniformBuffer() = default;

        virtual void SetData(const void* data, Uint offset = 0) const = 0;
        virtual void SetData(const Buffer& data, Uint offset = 0) const = 0;
        virtual const Buffer& GetData() const = 0;
        virtual Uint GetSize() const = 0;

        static Ref<UniformBuffer> Create(Uint size);
    };
} // namespace Surge