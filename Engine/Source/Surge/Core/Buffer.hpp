// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"

namespace Surge
{
    struct Buffer
    {
        void* Data;
        Uint Size;

        Buffer()
            : Data(nullptr), Size(0) {}

        Buffer(void* data, Uint size)
            : Data(data), Size(size) {}

        void Allocate(Uint size)
        {
            delete[] Data;
            Data = nullptr;

            if (size == 0)
                return;

            Data = new byte[size];
            Size = size;
        }

        void Release()
        {
            delete[] Data;
            Data = nullptr;
            Size = 0;
        }

        void ZeroInitialize()
        {
            if (Data)
                memset(Data, 0, Size);
        }

        template <typename T>
        T& Read(Uint offset = 0)
        {
            return *(T*)((byte*)Data + offset);
        }

        byte* ReadBytes(Uint size, Uint offset)
        {
            SG_ASSERT(offset + size <= Size, "Buffer overflow!");
            byte* buffer = new byte[size];
            std::memcpy(buffer, (byte*)Data + offset, size);
            return buffer;
        }

        void Write(void* data, Uint size, Uint offset = 0)
        {
            SG_ASSERT(offset + size <= Size, "Buffer overflow!");
            std::memcpy((byte*)Data + offset, data, size);
        }

        operator bool() const
        {
            return Data;
        }

        byte& operator[](int index)
        {
            return ((byte*)Data)[index];
        }

        byte operator[](int index) const
        {
            return ((byte*)Data)[index];
        }

        template <typename T>
        T* As()
        {
            return (T*)Data;
        }

        Uint GetSize() const
        {
            return Size;
        }
    };

} // namespace Surge
