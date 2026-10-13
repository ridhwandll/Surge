// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include <cstring>
#include <type_traits>

namespace Surge
{
    // Writers
    template <typename T>
    static void WriteData(Vector<Byte>& buffer, const T& data)
    {
        const Byte* ptr = reinterpret_cast<const Byte*>(&data);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }

    template <typename T>
    static void WriteDataArray(Vector<Byte>& buffer, const T* data, size_t count)
    {
        if(count == 0)
            return;
        const Byte* ptr = reinterpret_cast<const Byte*>(data);
        buffer.insert(buffer.end(), ptr, ptr + (sizeof(T) * count));
    }

    [[maybe_unused]] static void WriteStr(Vector<Byte>& buffer, const String& s)
    {
        Uint len = static_cast<Uint>(s.size());
        WriteData(buffer, len);
        WriteDataArray(buffer, s.data(), s.size());
    }

    // BinaryReader
    // Bounds checked cursor over a file loaded in memory (cooked files can be truncated or stale, the runtime must never read past them).
    // A read that does not fit zero-fills its output and sets Failed, so callers can read a whole block and check Failed once.
    // Check counts read from the file with Fits() before allocating from them
    struct BinaryReader
    {
        const Byte* Ptr = nullptr;
        const Byte* End = nullptr;
        bool Failed = false;

        BinaryReader(const Byte* data, size_t size)
            : Ptr(data), End(data + size) {}

        size_t Remaining() const { return Failed ? 0 : static_cast<size_t>(End - Ptr); }

        // True if count elements of elementSize bytes are still left in the buffer
        bool Fits(size_t count, size_t elementSize) const { return elementSize == 0 || count <= Remaining() / elementSize; }

        bool Skip(size_t bytes)
        {
            if(!Fits(bytes, 1))
            {
                Failed = true;
                return false;
            }
            Ptr += bytes;
            return true;
        }

        template <typename T>
        void Read(T& data)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if(!Fits(1, sizeof(T)))
            {
                Failed = true;
                data = T {};
                return;
            }
            memcpy(&data, Ptr, sizeof(T));
            Ptr += sizeof(T);
        }

        template <typename T>
        void ReadArray(T* data, size_t count)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if(count == 0)
                return;
            if(!Fits(count, sizeof(T)))
            {
                Failed = true;
                memset(static_cast<void*>(data), 0, sizeof(T) * count);
                return;
            }
            memcpy(data, Ptr, sizeof(T) * count);
            Ptr += sizeof(T) * count;
        }

        String ReadStr()
        {
            Uint len = 0;
            Read(len);
            if(!Fits(len, 1))
            {
                Failed = true;
                return {};
            }
            String s(reinterpret_cast<const char*>(Ptr), len);
            Ptr += len;
            return s;
        }
    };
}
