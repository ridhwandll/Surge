// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Core/Window.hpp"

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
        bool mIsOpen = false;

        HWND mHwnd;
        static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM lparam, LPARAM wparam);
    };
}