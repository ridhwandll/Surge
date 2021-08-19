// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Window.hpp"

#ifdef SURGE_WIN32
	#include "Platform/Win32/Win32Window.hpp"
#endif

namespace Surge
{
	Window* Window::Create(int width, int height, const std::string& title)
	{
#ifdef SURGE_WIN32
		return new Win32Window(width, height, title);
#endif
	}
}