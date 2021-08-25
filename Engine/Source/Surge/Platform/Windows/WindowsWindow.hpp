// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    class SURGE_API WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowData& windowData);
        virtual ~WindowsWindow() override;

        virtual bool IsOpen() const override { return mIsOpen; };
        virtual void Update() override;
        virtual void RegisterApplication(Application* application) override { mApplication = application; }
    private:
        void ApplyFlags();
    private:
        Application* mApplication = nullptr;
        bool mIsOpen = false;
        HWND mWin32Window;
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM lparam, LPARAM wparam);
    };
}