// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/Filesystem.hpp"
#include <fstream>

namespace Surge::Filesystem
{
    String ReadFile(const Path& path)
    {
        String result;
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(result.data(), size);
            }
            else
            {
                SG_ASSERT_INTERNAL("Cannot open path \"{0}\"", path);
            }
            in.close();
        }

        return result;
    }
}
