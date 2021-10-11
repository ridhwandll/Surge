// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"
#include <windowsx.h>

namespace Surge
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowData& windowData);
        virtual ~WindowsWindow() override;

        virtual bool IsOpen() const override { return mIsOpen; };
        virtual void Update() override;
        virtual void Minimize() override
        {
            ShowWindow(mWin32Window, SW_MINIMIZE);
            mWindowState = WindowState::Minimized;
        }
        virtual void Maximize() override { ShowWindow(mWin32Window, SW_MAXIMIZE); }
        virtual void RestoreFromMaximize() override { ShowWindow(mWin32Window, SW_SHOWNORMAL); }
        virtual void RegisterEventCallback(std::function<void(Event&)> eventCallback) override { mEventCallback = eventCallback; }
        virtual bool IsWindowMaximized() const override { return IsMaximized(mWin32Window); }
        virtual bool IsWindowMinimized() const override { return IsIconic(mWin32Window); }

        virtual String GetTitle() const override { return mWindowData.Title; }
        virtual void SetTitle(const String& name) override;

        virtual glm::vec2 GetPos() const override;
        virtual void SetPos(const glm::vec2& pos) const override;

        virtual glm::vec2 GetSize() const override;
        virtual void SetSize(const glm::vec2& size) const override;

        virtual WindowState GetWindowState() const override { return mWindowState; }
        virtual void ShowConsole(bool show) const override;
        virtual void* GetNativeWindowHandle() override { return mWin32Window; }

    private:
        void ApplyFlags();
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM lParam, LPARAM wParam);

    private:
        std::function<void(Event&)> mEventCallback;
        WindowState mWindowState;
        bool mIsOpen = false;
        HWND mWin32Window;
    };
} // namespace Surge
