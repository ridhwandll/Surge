// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Window.hpp"

#ifdef SURGE_WINDOWS
    #include "Surge/Platform/Windows/WindowsWindow.hpp"
#endif

namespace Surge
{
    Scope<Window> Window::Create(const WindowData& windowData)
    {
#ifdef SURGE_WINDOWS
        return CreateScope<WindowsWindow>(windowData);
#else
        __debugbreak(); //TODO(Rid): Assertions
#endif
    }
}
