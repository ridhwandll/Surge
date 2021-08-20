// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include "Core/Window.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Surge
{
    class Win32Window : public Window
    {
    public:
        Win32Window(int width, int height, const String& title);
        virtual ~Win32Window() override;

        virtual bool IsOpen() const override { return mIsOpen; };
        virtual void Update() override;
    private:
        bool mIsOpen = 1;
        
        HWND mHwnd;
        static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM lparam, LPARAM wparam);
    };
}