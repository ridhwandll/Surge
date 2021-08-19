// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Win32Window.hpp"

namespace Surge
{
	Win32Window::Win32Window(int width, int height, const std::string& title)
	{
		mWidth = width;
		mHeight = height;
		mTitle = title;

		WNDCLASSA windowClass = {};
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.lpfnWndProc = Win32Window::WindowProc;
		windowClass.lpszClassName = title.c_str();
		windowClass.hCursor = LoadCursor(0, IDC_ARROW);

		RegisterClassA(&windowClass);

		mHwnd = CreateWindowA(
			windowClass.lpszClassName,
			title.c_str(),
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			mWidth, mHeight,
			0, 0,
			windowClass.hInstance,
			this
		);
	}

	Win32Window::~Win32Window()
	{
		DestroyWindow(mHwnd);
	}

	void Win32Window::Update()
	{
		MSG msg;
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// NOTE(milo): required or the program will take 25% cpu usage
		Sleep(1);
	}

	LRESULT Win32Window::WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_CREATE:
		{
			LPCREATESTRUCT const params = reinterpret_cast<LPCREATESTRUCT>(lparam);
			Win32Window* const wnd = reinterpret_cast<Win32Window* const>(params->lpCreateParams);

			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
			break;
		}
		case WM_CLOSE:
		{
			Win32Window* wnd = reinterpret_cast<Win32Window*>(GetWindowLongPtr(window, GWLP_USERDATA));
			wnd->mIsOpen = 0;

			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(window, msg, wparam, lparam);
		}
	}
}