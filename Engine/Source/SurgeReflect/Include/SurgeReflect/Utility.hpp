// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <string>

namespace SurgeReflect::Utility
{
    inline int64_t GenerateStringHash(const std::string& s)
    {
        int64_t result = 0;
        const int p = 31;
        const int m = 1e9 + 9;
        long long pPow = 1;
        for (char c : s)
        {
            result = (result + (c - 'a' + 1) * pPow) % m;
            pPow = (pPow * p) % m;
        }
        return result;
    }

} // namespace SurgeReflect::Utility