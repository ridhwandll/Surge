// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Application.hpp"
#include "Surge/Core/Defines.hpp"
#include <functional>
#include <glm/glm.hpp>

namespace Surge
{
    enum class WindowFlags
    {
        // TODO: Add more falgs
        Minimized = BIT(1),
        Maximized = BIT(2),
        CreateDefault = BIT(3)
    };
    MAKE_BIT_ENUM(WindowFlags);

    enum class WindowState
    {
        Normal = 0,
        Minimized
    };

    struct WindowData
    {
        WindowData(Uint width, Uint height, const String& title, WindowFlags flags = WindowFlags::CreateDefault) : Width(width), Height(height), Title(title), Flags(flags) {}
        WindowData() : Width(1280), Height(720), Title("Surge Window"), Flags(WindowFlags::CreateDefault) {}
        ~WindowData() = default;

        Uint Width;
        Uint Height;
        String Title;
        WindowFlags Flags;
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        virtual bool IsOpen() const = 0;
        virtual void Update() = 0;
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void RegisterEventCallback(std::function<void(Event&)> eventCallback) = 0;

        // Get/Set the title Window(in pixels)
        virtual String GetTitle() const = 0;
        virtual void SetTitle(const String& name) = 0;

        // Get/Set the position of the Window(in pixels)
        virtual glm::vec2 GetPos() const = 0;
        virtual void SetPos(const glm::vec2& pos) const = 0;

        // Get/Set the size of the Window(in pixels)
        virtual glm::vec2 GetSize() const = 0;
        virtual void SetSize(const glm::vec2& size) const = 0;

        virtual WindowState GetWindowState() const = 0;
        virtual void ShowConsole(bool show) const = 0;
        virtual void* GetNativeWindowHandle() = 0;

        const WindowData& GetData() const { return mWindowData; }

    protected:
        WindowData mWindowData;
    };
} // namespace Surge
