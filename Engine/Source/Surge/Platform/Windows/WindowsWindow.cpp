// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Platform/Windows/WindowsWindow.hpp"
#include <windowsx.h>

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
        wc.lpszClassName = "Surge Win32Window";
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
        if (mWin32Window)
            Log<LogSeverity::Info>("Created {0} ({1}, {2})", mWindowData.Title, mWindowData.Width, mWindowData.Height);
        else
            Log<LogSeverity::Error>("WindowsWindow creation failure!");
    }

    WindowsWindow::~WindowsWindow()
    {
        DestroyWindow(mWin32Window);
    }

    void WindowsWindow::Update()
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void WindowsWindow::Minimize()
    {
        ShowWindow(mWin32Window, SW_MINIMIZE);
    }

    void WindowsWindow::Maximize()
    {
        ShowWindow(mWin32Window, SW_MAXIMIZE);
    }

    void WindowsWindow::SetTitle(const String& name)
    {
        mWindowData.Title = name;
        SetWindowText(mWin32Window, mWindowData.Title.c_str());
    }

    Surge::Pair<float, float> WindowsWindow::GetPos() const
    {
        POINT pos = { 0, 0 };
        ClientToScreen(mWin32Window, &pos);
        return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
    }

    void WindowsWindow::SetPos(const Pair<float, float>& pos) const
    {
        RECT rect = { (LONG)pos.Data1, (LONG)pos.Data2, (LONG)pos.Data1, (LONG)pos.Data2 };
        SetWindowPos(mWin32Window, nullptr, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
    }

    Surge::Pair<float, float> WindowsWindow::GetSize() const
    {
        RECT area;
        GetClientRect(mWin32Window, &area);
        return { static_cast<float>(area.right), static_cast<float>(area.bottom) };
    }

    void WindowsWindow::SetSize(const Pair<float, float>& size) const
    {
        RECT rect = { 0, 0, (LONG)size.Data1, (LONG)size.Data2 };
        SetWindowPos(mWin32Window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    }

    void WindowsWindow::ShowConsole(bool show) const
    {
        if (show)
            ShowWindow(GetConsoleWindow(), SW_SHOW);
        else
            ShowWindow(GetConsoleWindow(), SW_HIDE);
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

    LRESULT WindowsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
        {
            LPCREATESTRUCT const params = reinterpret_cast<LPCREATESTRUCT>(lParam);
            WindowsWindow* const wnd = reinterpret_cast<WindowsWindow* const>(params->lpCreateParams);
            wnd->mIsOpen = true;

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
            break;
        }
        case WM_CLOSE:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            WindowClosedEvent event;
            data->mApplication->OnEvent(event);
            data->mIsOpen = false;
            Surge::Close();
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            data->mWindowData.Width = (UINT)LOWORD(lParam);
            data->mWindowData.Height = (UINT)HIWORD(lParam);

            WindowResizeEvent event((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            if (data->mApplication != nullptr)
                data->mApplication->OnEvent(event);
            break;
        }
        case WM_KEYUP:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            KeyReleasedEvent event(static_cast<KeyCode>(wParam));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_CHAR:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            KeyTypedEvent event(static_cast<KeyCode>(wParam));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_KEYDOWN:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            int repeatCount = (lParam & 0xffff);
            KeyPressedEvent event(static_cast<KeyCode>(wParam), repeatCount);
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_MOUSEMOVE:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseMovedEvent event((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseScrolledEvent event((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonPressedEvent event(static_cast<MouseCode>(VK_LBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_LBUTTONUP:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonReleasedEvent event(static_cast<MouseCode>(VK_LBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_MBUTTONDOWN:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonPressedEvent event(static_cast<MouseCode>(VK_MBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_MBUTTONUP:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonReleasedEvent event(static_cast<MouseCode>(VK_MBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_RBUTTONDOWN:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonPressedEvent event(static_cast<MouseCode>(VK_RBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        case WM_RBUTTONUP:
        {
            WindowsWindow* data = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            MouseButtonReleasedEvent event(static_cast<MouseCode>(VK_RBUTTON));
            data->mApplication->OnEvent(event);
            break;
        }
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        return 0;
    }
}
