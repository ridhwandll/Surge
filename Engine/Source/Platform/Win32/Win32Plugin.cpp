// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Win32Plugin.hpp"

namespace Surge
{
    Win32Plugin::Win32Plugin(const String& path)
    {
        mModule = LoadLibrary(path.c_str());
    }

    Win32Plugin::~Win32Plugin()
    {
        FreeLibrary(mModule);
    }

    void* Win32Plugin::LoadFunction(const String& name)
    {
        void* pointer = GetProcAddress(mModule, name.c_str());
        return pointer;
    }
}
