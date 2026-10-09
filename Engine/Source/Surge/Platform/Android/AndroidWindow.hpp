// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#ifdef SURGE_PLATFORM_ANDROID

#include "Surge/Core/Window/Window.hpp"
#include "Surge/Platform/Android/AndroidApp.hpp"

#include <android/native_window.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <game-activity/GameActivityEvents.h>

namespace Surge
{
    class AndroidWindow : public Window
    {
    public:
        explicit AndroidWindow(const WindowDesc& windowData);
        virtual ~AndroidWindow() override = default;

        virtual void Update() override;
        virtual void Minimize() override {}
        virtual void Maximize() override {}
        virtual void RestoreFromMaximize() override {}
        virtual void RegisterEventCallback(std::function<void(Event&)> eventCallback) override { mEventCallback = eventCallback; }

        virtual bool IsWindowMaximized() const override { return false; }
        virtual bool IsWindowMinimized() const override { return !IsVisible(); }

        virtual String GetTitle() const override { return mWindowData.Title; }
        virtual void SetTitle(const String& name) override { mWindowData.Title = name; }

        virtual glm::vec2 GetPos() const override { return { 0.0f, 0.0f }; }
        virtual void SetPos(const glm::vec2&) const override {}

        virtual glm::vec2 GetSize() const override;
        virtual void SetSize(const glm::vec2& size) const override;

        virtual WindowState GetWindowState() const override { return IsVisible() ? WindowState::NORMAL : WindowState::MINIMIZED; }
        virtual void ShowConsole(bool) const override {}
        virtual void* GetNativeWindowHandle() override { return static_cast<void*>(mNativeWindow); }

    private:
        bool IsVisible() const { return mNativeWindow != nullptr && mResumed; }

        static void HandleAppCmd(android_app* app, int32_t cmd);
        static void HandleMotionEvent(AndroidWindow* window, const GameActivityMotionEvent* event);
        static void HandleKeyEvent(AndroidWindow* window, const GameActivityKeyEvent* event);

        // Input helpers (AndroidWindow.cpp)
        void Emit(Event& e) { if (mEventCallback) mEventCallback(e); }
        void HandleTouchMotion(const GameActivityMotionEvent* event, int32_t actionMasked, Uint pointerIndex);
        void HandleMouseMotion(const GameActivityMotionEvent* event, int32_t actionMasked, Uint pointerIndex);
        void HandleGamepadMotion(const GameActivityMotionEvent* event);
        void ReleaseAllPointers(); // Focus loss: cancel touches, release mouse buttons
        int GamepadSlot(int32_t deviceId);

    private:
        std::function<void(Event&)> mEventCallback;
        ANativeWindow* mNativeWindow = nullptr;
        bool mResumed = true; // APP_CMD_RESUME was already handled by android_main before the window existed
        bool mDestroyFired = false;

        // Active touch contacts (pointer id + last position), the primary one also drives the emulated mouse
        static constexpr uint32_t MAX_POINTERS = 16;
        struct PointerState { int32_t ID; float X, Y; };
        PointerState mPointers[MAX_POINTERS] = {};
        uint32_t mPointerCount = 0;
        int32_t mPrimaryPointer = -1;

        // Real mouse (USB/Bluetooth, DeX)
        int32_t mMouseButtons = 0;
        float mMouseX = 0.0f, mMouseY = 0.0f;

        // Gamepads: Android device id per Surge gamepad slot, last d-pad hat values
        int32_t mGamepadDevices[4] = { -1, -1, -1, -1 };
        float mGamepadHat[4][2] = {};

        // Whether the current back press closes the app, decided when it goes down (the game may react to that press
        // and change its mind, e.g. close a dialog, the release must still do what the press meant)
        bool mBackExitsOnRelease = false;
    };

} // namespace Surge

#endif // SURGE_PLATFORM_ANDROID