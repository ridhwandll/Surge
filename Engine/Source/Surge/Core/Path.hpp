// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/String.hpp"
#include <filesystem>
#include <fmt/format.h>

namespace Surge
{
    class Path
    {
    public:
        Path() = default;
        Path(const char* cstr)
            : mPathStr(cstr) {}
        Path(const String& str)
            : mPathStr(str) {}
        Path(const std::filesystem::path& path)
            : mPathStr(path.string()) {}

        ~Path() = default;

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