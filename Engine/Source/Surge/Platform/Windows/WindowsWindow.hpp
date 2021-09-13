// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowData& windowData);
        virtual ~WindowsWindow() override;

        virtual bool IsOpen() const override { return mIsOpen; };
        virtual void Update() override;
        virtual void Minimize() override;
        virtual void Maximize() override;
        virtual void RegisterEventCallback(std::function<void(Event&)> eventCallback) override { mEventCallback = eventCallback; }

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
    private:
        std::function<void(Event&)> mEventCallback;
        WindowState mWindowState;
        bool mIsOpen = false;
        HWND mWin32Window;
        RenderContext* mRenderingContext;
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM lparam, LPARAM wparam);
    };
}
