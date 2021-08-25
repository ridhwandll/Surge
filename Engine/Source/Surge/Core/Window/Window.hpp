// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"

namespace Surge
{
    enum class WindowFlags
    {
        NonResizeable = BIT(0),
        NoDecoration  = BIT(1),
        Minimized     = BIT(2),
        Maximized     = BIT(3),
        CreateDefault = BIT(4)
    };
    MAKE_BIT_ENUM(WindowFlags);

    struct WindowData
    {
        WindowData(Uint width, Uint height, const String& title, WindowFlags flags = WindowFlags::CreateDefault)
            : Width(width), Height(height), Title(title), Flags(flags) {}

        WindowData()
            : Width(1280), Height(720), Title("Surge Window"), Flags(WindowFlags::CreateDefault) {}

        ~WindowData() = default;

        Uint Width;
        Uint Height;
        String Title;
        WindowFlags Flags;
    };

    class SURGE_API Window
    {
    public:
        virtual ~Window() = default;

        virtual bool IsOpen() const = 0;
        virtual void Update() = 0;

        Uint GetWidth() const { return mWindowData.Width; }
        Uint GetHeight() const { return mWindowData.Height; }
        const WindowData& GetData() const { return mWindowData; }
        const String& GetTitle() const { return mWindowData.Title; }

        static Scope<Window> Create(const WindowData& windowData);
    protected:
        WindowData mWindowData;
    };
}