// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Plugin.hpp"

#ifdef SURGE_WIN32
    #include "Platform/Win32/Win32Plugin.hpp"
#endif

namespace Surge
{
    Ref<Plugin> Plugin::Create(const Surge::String& path)
    {
#ifdef SURGE_WIN32
        return CreateRef<Win32Plugin>(path);
#endif
    }
}