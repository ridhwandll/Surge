// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"

namespace Surge
{
    class FileDialog
    {
    public:
        static String OpenFile(const char* filter);
        static Vector<String> OpenFiles(const char* filter); // Multi-selection, empty when canceled
        static String SaveFile(const char* filter, const char* defaultName = "");
        static String ChooseFolder();
    };

} // namespace Surge::FileDialog
