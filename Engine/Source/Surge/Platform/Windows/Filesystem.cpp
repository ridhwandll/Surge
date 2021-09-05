// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/Filesystem.hpp"
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
}
