// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/Filesystem.hpp"
#include <filesystem>
#include <fstream>

namespace Surge::Filesystem
{
    String ReadFile(const Path& path)
    {
        String result;
        std::ifstream inStream(path, std::ios::in | std::ios::binary);
        if (inStream)
        {
            inStream.seekg(0, std::ios::end);
            size_t size = inStream.tellg();
            if (size != -1)
            {
                result.resize(size);
                inStream.seekg(0, std::ios::beg);
                inStream.read(result.data(), size);
            }
            inStream.close();
        }
        else
        {
            SG_ASSERT_INTERNAL("Cannot open path \"{0}\"", path);
        }

        return result;
    }

    String RemoveExtension(const Path& path)
    {
        size_t lastindex = path.find_last_of(".");
        String rawName = path.substr(0, lastindex);
        return rawName;
    }

    String GetNameWithExtension(const Path& assetFilepath) { return std::filesystem::path(assetFilepath).filename().string(); }

    String GetNameWithoutExtension(const Path& assetFilepath)
    {
        String name;
        auto lastSlash = assetFilepath.find_last_of("/\\");
        lastSlash = lastSlash == String::npos ? 0 : lastSlash + 1;
        auto lastDot = assetFilepath.rfind('.');
        auto count = lastDot == String::npos ? assetFilepath.size() - lastSlash : lastDot - lastSlash;
        name = assetFilepath.substr(lastSlash, count);
        return name;
    }

    bool Exists(const Path& path)
    {
        std::ifstream exists(path);
        if (exists.is_open())
            return true;

        return false;
    }
} // namespace Surge::Filesystem
