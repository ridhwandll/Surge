// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Core/Input/KeyCodes.hpp"
#include "Surge/Core/Input/MouseCodes.hpp"
#include <sstream>

namespace Surge
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        WindowSurfaceDestroyed,
        WindowSurfaceCreated,
        TouchPressed,
        TouchMoved,
        TouchReleased,
        FilesDropped
    };

    class Event
    {
    public:
        Event() = default;
        virtual ~Event() = default;
        virtual String ToString() const = 0;
        virtual const char* GetName() const = 0;
        virtual EventType GetEventType() const = 0;

    public:
        bool Handled = false; // Set by a system that consumed the event (the UI closing a popup on Back), the platform layer may check it
    };

#define EVENT_CLASS_TYPE(type)                                                  \
    static EventType GetStaticType() { return EventType::type; }                \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

    // Key Events
    class KeyEvent : public Event
    {
    public:
        Key GetKeyCode() const { return mKeyCode; }

    protected:
        KeyEvent(const Key keycode) : mKeyCode(keycode) {}

        Key mKeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(const Key keycode, const uint16_t repeatCount) : KeyEvent(keycode), mRepeatCount(repeatCount) {}

        uint16_t GetRepeatCount() const { return mRepeatCount; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << (KeyCode)mKeyCode << " (" << mRepeatCount << " repeats)";
            return ss.str();
        }
        EVENT_CLASS_TYPE(KeyPressed);

    private:
        uint16_t mRepeatCount;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(const KeyCode keycode) : KeyEvent((Key)keycode) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << (KeyCode)mKeyCode;
            return ss.str();
        }
        EVENT_CLASS_TYPE(KeyReleased);
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(const KeyCode keycode) : KeyEvent((Key)keycode) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << (KeyCode)mKeyCode;
            return ss.str();
        }
        EVENT_CLASS_TYPE(KeyTyped);
    };

    // Mouse Events
    // fromTouch: the OS (or the Android window) synthesized this mouse event from the primary touch, the touch itself
    // also arrives as a Touch*Event, so systems that handle touches must ignore these to not process it twice
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(const float x, const float y, bool fromTouch = false) : mMouseX(x), mMouseY(y), mFromTouch(fromTouch) {}

        float GetX() const { return mMouseX; }
        float GetY() const { return mMouseY; }
        bool IsFromTouch() const { return mFromTouch; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << mMouseX << ", " << mMouseY;
            return ss.str();
        }
        EVENT_CLASS_TYPE(MouseMoved);

    private:
        float mMouseX, mMouseY;
        bool mFromTouch;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(const float delta) : mDelta(delta) {}

        float GetDelta() const { return mDelta; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << mDelta;
            return ss.str();
        }
        EVENT_CLASS_TYPE(MouseScrolled);

    private:
        float mDelta;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseCode GetMouseButton() const { return mButton; }

        float GetX() const { return mMouseX; }
        float GetY() const { return mMouseY; }
        bool IsFromTouch() const { return mFromTouch; }

    protected:
        MouseButtonEvent(float mouseX, float mouseY, const MouseCode button, bool fromTouch)
            : mButton(button), mMouseX(mouseX), mMouseY(mouseY), mFromTouch(fromTouch) {}

        MouseCode mButton;
        float mMouseX, mMouseY;
        bool mFromTouch;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(float mouseX, float mouseY, const MouseCode button, bool fromTouch = false)
            : MouseButtonEvent(mouseX, mouseY, button, fromTouch) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << mButton;
            return ss.str();
        }
        EVENT_CLASS_TYPE(MouseButtonPressed);
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(float mouseX, float mouseY, const MouseCode button, bool fromTouch = false)
            : MouseButtonEvent(mouseX, mouseY, button, fromTouch) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << mButton;
            return ss.str();
        }
        EVENT_CLASS_TYPE(MouseButtonReleased);
    };

    // Touch Events
    // PointerID is the platform pointer id (Android pointer id, Win32 pointer id): unique while the contact lasts
    // Coordinates are window client pixels, the same space as mouse events
    class TouchEvent : public Event
    {
    public:
        int64_t GetPointerID() const { return mPointerID; }
        float GetX() const { return mX; }
        float GetY() const { return mY; }
        float GetPressure() const { return mPressure; }

    protected:
        TouchEvent(int64_t pointerID, float x, float y, float pressure) : mPointerID(pointerID), mX(x), mY(y), mPressure(pressure) {}

        int64_t mPointerID;
        float mX, mY, mPressure;
    };

    class TouchPressedEvent : public TouchEvent
    {
    public:
        TouchPressedEvent(int64_t pointerID, float x, float y, float pressure = 1.0f) : TouchEvent(pointerID, x, y, pressure) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "TouchPressedEvent: " << mPointerID << " (" << mX << ", " << mY << ")";
            return ss.str();
        }
        EVENT_CLASS_TYPE(TouchPressed);
    };

    class TouchMovedEvent : public TouchEvent
    {
    public:
        TouchMovedEvent(int64_t pointerID, float x, float y, float pressure = 1.0f) : TouchEvent(pointerID, x, y, pressure) {}

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "TouchMovedEvent: " << mPointerID << " (" << mX << ", " << mY << ")";
            return ss.str();
        }
        EVENT_CLASS_TYPE(TouchMoved);
    };

    // Canceled: the OS took the contact away (gesture, focus loss), it must not count as a release/click
    class TouchReleasedEvent : public TouchEvent
    {
    public:
        TouchReleasedEvent(int64_t pointerID, float x, float y, bool canceled = false) : TouchEvent(pointerID, x, y, 0.0f), mCanceled(canceled) {}

        bool IsCanceled() const { return mCanceled; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "TouchReleasedEvent: " << mPointerID << " (" << mX << ", " << mY << ")" << (mCanceled ? " canceled" : "");
            return ss.str();
        }
        EVENT_CLASS_TYPE(TouchReleased);

    private:
        bool mCanceled;
    };

    // App Events
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(Uint width, Uint height) : mWidth(width), mHeight(height) {}

        Uint GetWidth() const { return mWidth; }
        Uint GetHeight() const { return mHeight; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
            return ss.str();
        }
        EVENT_CLASS_TYPE(WindowResize);

    private:
        Uint mWidth, mHeight;
    };

    // The window lost keyboard/pointer focus: held keys, buttons and touches will never see their release
    class WindowLostFocusEvent : public Event
    {
    public:
        WindowLostFocusEvent() {}

        virtual String ToString() const override { return "WindowLostFocusEvent"; }
        EVENT_CLASS_TYPE(WindowLostFocus);
    };

    // Files/folders dropped onto the window from the OS (only sent when the window opted in with Window::SetFileDropEnabled)
    class FilesDroppedEvent : public Event
    {
    public:
        FilesDroppedEvent(Vector<String>&& paths, float x, float y)
            : mPaths(std::move(paths)), mX(x), mY(y) {}

        const Vector<String>& GetPaths() const { return mPaths; }
        float GetX() const { return mX; } // Drop point, client area pixels
        float GetY() const { return mY; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "FilesDroppedEvent: " << mPaths.size() << " path(s) at " << mX << ", " << mY;
            return ss.str();
        }
        EVENT_CLASS_TYPE(FilesDropped);

    private:
        Vector<String> mPaths;
        float mX, mY;
    };

    class WindowClosedEvent : public Event
    {
    public:
        WindowClosedEvent() {}

        virtual String ToString() const override { return "WindowCloseEvent"; }
        EVENT_CLASS_TYPE(WindowClose);
    };

    // Android: the native window (and with it the Vulkan surface) is destroyed when the app goes to the background
    // Fired before the native window becomes invalid, renderers must release everything that references it
    class WindowSurfaceDestroyedEvent : public Event
    {
    public:
        WindowSurfaceDestroyedEvent() {}

        virtual String ToString() const override { return "WindowSurfaceDestroyedEvent"; }
        EVENT_CLASS_TYPE(WindowSurfaceDestroyed);
    };

    // Fired once a new native window is available (e.g. when the app returns to the foreground)
    class WindowSurfaceCreatedEvent : public Event
    {
    public:
        WindowSurfaceCreatedEvent(Uint width, Uint height) : mWidth(width), mHeight(height) {}

        Uint GetWidth() const { return mWidth; }
        Uint GetHeight() const { return mHeight; }

        virtual String ToString() const override
        {
            std::stringstream ss;
            ss << "WindowSurfaceCreatedEvent: " << mWidth << ", " << mHeight;
            return ss.str();
        }
        EVENT_CLASS_TYPE(WindowSurfaceCreated);

    private:
        Uint mWidth, mHeight;
    };

    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event)
            : mEvent(event) {}

        template <typename T, typename F>
        void Dispatch(const F& func)
        {
            if (mEvent.GetEventType() == T::GetStaticType())
                func(static_cast<T&>(mEvent));
        }

    private:
        Event& mEvent;
    };

} // namespace Surge
