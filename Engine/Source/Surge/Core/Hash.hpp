// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <cstdint>

namespace Surge
{
    using HashCode = int64_t;
    class Hash
    {
    public:
        template <typename T>
        inline HashCode Generate(const T& s);
    };

    template <typename T>
    inline HashCode Hash::Generate(const T& s)
    {
        static_assert(false);
    }

    template <>
    inline HashCode Hash::Generate(const String& s)
    {
        HashCode result = 0;
        // Based on: https://cp-algorithms.com/string/string-hashing.html
        const int p = 53;
        const int m = 1e9 + 9;
        long long pPow = 1;
        for (char c: s)
        {
            result = (result + (c - 'a' + 1) * pPow) % m;
            pPow = (pPow * p) % m;
        }
        return result;
    }
} // namespace Surge
