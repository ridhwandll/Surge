// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class UniformBuffer : public RefCounted
    {
    public:
        virtual ~UniformBuffer() = default;

        virtual void SetData(const void* data, Uint size, Uint offset = 0) = 0;
        virtual Uint GetSize() const = 0;
        virtual Uint GetBinding() const = 0;

        static Ref<UniformBuffer> Create(Uint size, Uint binding);
    };

} // namespace Surge
