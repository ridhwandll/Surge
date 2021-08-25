// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Application.hpp"

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
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void RegisterApplication(Application* application) = 0;

        // Get/Set the title Window(in pixels)
        virtual String GetTitle() const = 0;
        virtual void SetTitle(const String& name) = 0;

        // Get/Set the position of the Window(in pixels)
        virtual Pair<float, float> GetPos() const = 0;
        virtual void SetPos(const Pair<float, float>& pos) const = 0;

        // Get/Set the size of the Window(in pixels)
        virtual Pair<float, float> GetSize() const = 0;
        virtual void SetSize(const Pair<float, float>& size) const = 0;

        // Turn in/off the console window
        virtual void ShowConsole(bool show) const = 0;

        const WindowData& GetData() const { return mWindowData; }
        static Scope<Window> Create(const WindowData& windowData);
    protected:
        WindowData mWindowData;
    };
}