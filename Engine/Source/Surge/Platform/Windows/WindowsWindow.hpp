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
        virtual void Minimize() override;
        virtual void Maximize() override;
        virtual void RegisterApplication(Application* application) override { mApplication = application; }

        virtual String GetTitle() const override { return mWindowData.Title; }
        virtual void SetTitle(const String& name) override;

        virtual Pair<float, float> GetPos() const override;
        virtual void SetPos(const Pair<float, float>& pos) const override;

        virtual Pair<float, float> GetSize() const override;
        virtual void SetSize(const Pair<float, float>& size) const override;

        virtual void ShowConsole(bool show) const override;
    private:
        void ApplyFlags();
    private:
        Application* mApplication = nullptr;
        bool mIsOpen = false;
        HWND mWin32Window;
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM lparam, LPARAM wparam);
    };
}