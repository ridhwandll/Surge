// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Platform/Windows/WindowsWindow.hpp"

namespace Surge
{
    HINSTANCE hInstance;

    WindowsWindow::WindowsWindow(const WindowData& windowData)
    {
        mWindowData = windowData;
        hInstance = GetModuleHandle(nullptr);

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = WindowProc;
        wc.style = CS_CLASSDC;
        wc.hInstance = hInstance;
        wc.lpszClassName = "Electro Win32Window";
        wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
        wc.hIcon = nullptr;
        wc.hIconSm = wc.hIcon;
        wc.cbClsExtra = 0;

        if (!RegisterClassEx(&wc))
            Log<LogSeverity::Error>("Could not initialize the window class!");

        mWin32Window = CreateWindow(wc.lpszClassName, mWindowData.Title.c_str(),
            mWindowData.Flags & WindowFlags::NonResizeable ? WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : WS_OVERLAPPEDWINDOW,
            0, 0, mWindowData.Width, mWindowData.Height, nullptr, NULL, wc.hInstance, this);

        ApplyFlags();
        Log<LogSeverity::Info>("Created {0} ({1}, {2})", mWindowData.Title, mWindowData.Width, mWindowData.Height);
    }

    WindowsWindow::~WindowsWindow()
    {
        DestroyWindow(mWin32Window);
    }

    void WindowsWindow::Update()
    {
        MSG msg;
        while (PeekMessageA(&msg, mWin32Window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void WindowsWindow::ApplyFlags()
    {
        const WindowFlags& flags = mWindowData.Flags;

        if (!((flags & WindowFlags::Maximized) && (flags & WindowFlags::Minimized)))
        {
            if (flags & WindowFlags::Maximized)
                ShowWindow(mWin32Window, SW_SHOWMAXIMIZED);
            if (flags & WindowFlags::Minimized)
                ShowWindow(mWin32Window, SW_SHOWMINIMIZED);
            if (flags & WindowFlags::CreateDefault)
                ShowWindow(mWin32Window, SW_SHOWDEFAULT);
        }

        if (flags & WindowFlags::NoDecoration)
            SetWindowLong(mWin32Window, GWL_STYLE, 0);
    }

    LRESULT WindowsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_CREATE:
        {
            LPCREATESTRUCT const params = reinterpret_cast<LPCREATESTRUCT>(lparam);
            WindowsWindow* const wnd = reinterpret_cast<WindowsWindow* const>(params->lpCreateParams);
            wnd->mIsOpen = true;

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
            break;
        }
        case WM_CLOSE:
        {
            WindowsWindow* wnd = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            wnd->mIsOpen = false;

            Surge::Close();
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hWnd, msg, wparam, lparam);
        }

        return 0;
    }
}
