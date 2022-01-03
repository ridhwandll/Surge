// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/String.hpp"
#include <fmt/format.h>

namespace Surge
{
    class SURGE_API Path
    {
    public:
        Path() = default;
        Path(const char* cstr)
            : mPathStr(cstr) {}
        Path(const String& str)
            : mPathStr(str) {}

        ~Path() = default;

        Path ParentPath()
        {
            auto path = mPathStr.substr(0, mPathStr.find_last_of("/\\"));
            return path;
        }

        Path FileName()
        {
            String name;
            auto lastSlash = mPathStr.find_last_of("/\\");
            lastSlash = lastSlash == String::npos ? 0 : lastSlash + 1;
            auto lastDot = mPathStr.rfind('.');
            auto count = lastDot == String::npos ? mPathStr.size() - lastSlash : lastDot - lastSlash;
            name = mPathStr.substr(lastSlash, count);
            return name;
        }

        const String& Str() const { return mPathStr; }
        std::wstring WStr() const { return std::wstring(mPathStr.begin(), mPathStr.end()); }

        operator String() { return mPathStr; }
        operator const char*() const { return mPathStr.c_str(); }

        [[nodiscard]] friend bool operator==(const Path& left, const String& right) { return left.Str() == right; }
        [[nodiscard]] friend Path operator/(const Path& left, const Path& right) { return fmt::format("{0}/{1}", left.Str(), right.Str()); }
        [[nodiscard]] friend Path operator/(const Path& left, const String& right) { return fmt::format("{0}/{1}", left.Str(), right); }
        [[nodiscard]] friend Path operator/(const Path& left, const char* right) { return fmt::format("{0}/{1}", left.Str(), right); }

    private:
        String mPathStr;
    };

} // namespace Surge