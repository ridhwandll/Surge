// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Window.hpp"

#ifdef SURGE_WIN32
    #include "Platform/Win32/Win32Window.hpp"
#endif

namespace Surge
{
    Scope<Window> Window::Create(int width, int height, const String& title)
    {
#ifdef SURGE_WIN32
        return CreateScope<Win32Window>(width, height, title);
#else
        __debugbreak(); //TODO(Rid): Assertions
#endif
    }
}
