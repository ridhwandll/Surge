// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Input/KeyCodes.hpp"
#include "Surge/Core/Input/MouseCodes.hpp"
#include <sstream>

#define GET_NAME_IMPL(type) virtual const char* GetName() const override { return #type; }

namespace Surge
{
    class Event
    {
    public:
        Event() = default;
        virtual ~Event() = default;
        virtual String ToString() const = 0;
        virtual const char* GetName() const = 0;
    };

    // Key Events
    class KeyEvent : public Event
    {
    public:
        KeyCode GetKeyCode() const { return mKeyCode; }

    protected:
        KeyEvent(const KeyCode keycode)
            : mKeyCode(keycode) {}

        KeyCode mKeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(const KeyCode keycode, const uint16_t repeatCount)
            : KeyEvent(keycode), mRepeatCount(repeatCount) {}

        uint16_t GetRepeatCount() const { return mRepeatCount; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << mKeyCode << " (" << mRepeatCount << " repeats)";
            return ss.str();
        }
        GET_NAME_IMPL(KeyPressedEvent);
    private:
        uint16_t mRepeatCount;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(const KeyCode keycode)
            : KeyEvent(keycode) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << mKeyCode;
            return ss.str();
        }
        GET_NAME_IMPL(KeyReleasedEvent);
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(const KeyCode keycode)
            : KeyEvent(keycode) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << mKeyCode;
            return ss.str();
        }
        GET_NAME_IMPL(KeyTypedEvent);
    };

    // Mouse Events
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(const float x, const float y)
            : mMouseX(x), mMouseY(y) {}

        float GetX() const { return mMouseX; }
        float GetY() const { return mMouseY; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << mMouseX << ", " << mMouseY;
            return ss.str();
        }
        GET_NAME_IMPL(MouseMovedEvent);
    private:
        float mMouseX, mMouseY;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(const float delta)
            : mDelta(delta) {}

        float GetDelta() const { return mDelta; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << mDelta;
            return ss.str();
        }
        GET_NAME_IMPL(MouseScrolledEvent);
    private:
        float mDelta;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseCode GetMouseButton() const { return mButton; }

    protected:
        MouseButtonEvent(const MouseCode button)
            : mButton(button) {}

        MouseCode mButton;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(const MouseCode button)
            : MouseButtonEvent(button) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << mButton;
            return ss.str();
        }
        GET_NAME_IMPL(MouseButtonPressedEvent);
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(const MouseCode button)
            : MouseButtonEvent(button) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << mButton;
            return ss.str();
        }
        GET_NAME_IMPL(MouseButtonReleasedEvent);
    };

    // App Events
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(Uint width, Uint height)
            : mWidth(width), mHeight(height) {}

        Uint GetWidth() const { return mWidth; }
        Uint GetHeight() const { return mHeight; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
            return ss.str();
        }
        GET_NAME_IMPL(WindowResizeEvent);
    private:
        Uint mWidth, mHeight;
    };

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() {}

        virtual String ToString() const override
        {
            return "WindowCloseEvent";
        }
        GET_NAME_IMPL(WindowCloseEvent);
    };
}
