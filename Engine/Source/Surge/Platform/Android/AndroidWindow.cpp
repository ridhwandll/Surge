// Copyright (c) - SurgeTechnologies - All rights reserved
#ifdef SURGE_PLATFORM_ANDROID
#include "Surge/Platform/Android/AndroidWindow.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Surge/Core/Input/InputState.hpp"
#include <android/log.h>
#include <imgui.h>
#include <algorithm>

namespace Surge
{
    static AndroidWindow* sAndroidWindowInstance = nullptr;

    // MotionEvent.FLAG_CANCELED (API 33+): the contact was canceled (e.g. palm rejection), not in the NDK 27 headers
    static constexpr int32_t MOTION_EVENT_FLAG_CANCELED = 0x20;

    AndroidWindow::AndroidWindow(const WindowDesc& windowData)
    {
        mWindowData = windowData;
        sAndroidWindowInstance = this;

        android_app* app = Android::GAndroidApp;
        app->userData = this;
        app->onAppCmd = HandleAppCmd;

        // Pass nullptr to disable filtering and receive every axis
        android_app_set_motion_event_filter(app, nullptr);

        for(int32_t axis : { AMOTION_EVENT_AXIS_PRESSURE, AMOTION_EVENT_AXIS_VSCROLL, AMOTION_EVENT_AXIS_HSCROLL, AMOTION_EVENT_AXIS_Z, AMOTION_EVENT_AXIS_RZ,
            AMOTION_EVENT_AXIS_RX, AMOTION_EVENT_AXIS_RY, AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_RTRIGGER,
            AMOTION_EVENT_AXIS_BRAKE, AMOTION_EVENT_AXIS_GAS, AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y })
        {
            GameActivityPointerAxes_enableAxis(axis);
        }
        InputState::SetTouchSupported(true);

        // Surface may already be available if the window was created before Init
        mNativeWindow = app->window;
        if (mNativeWindow)
        {
            mWindowData.Width = static_cast<Uint>(ANativeWindow_getWidth(mNativeWindow));
            mWindowData.Height = static_cast<Uint>(ANativeWindow_getHeight(mNativeWindow));
        }

        Log<Severity::Info>("AndroidWindow created ({}x{})", mWindowData.Width, mWindowData.Height);
    }

    void AndroidWindow::Update()
    {
        android_app* app = Android::GAndroidApp;

        // Drain lifecycle / system events. While we have nothing to draw (no window, or paused) block on the looper instead of spinning: the next lifecycle command (RESUME, INIT_WINDOW, DESTROY) wakes us up
        int events = 0;
        android_poll_source* source = nullptr;
        while (true)
        {
            const int timeoutMs = (IsVisible() || app->destroyRequested) ? 0 : -1;
            if (ALooper_pollOnce(timeoutMs, nullptr, &events, reinterpret_cast<void**>(&source)) < 0)
                break;

            if (source)
                source->process(app, source);
        }

        // Check destroy request (set by glue after APP_CMD_DESTROY)
        if (app->destroyRequested && !mDestroyFired)
        {
            mDestroyFired = true;
            WindowClosedEvent e;
            if (mEventCallback)
                mEventCallback(e);
            return;
        }

        // Swap GameActivity input buffers, atomic hand-off from the input thread
        android_input_buffer* inputBuffer = android_app_swap_input_buffers(app);
        if (!inputBuffer)
            return;

        // Motion events (touch, stylus, joystick)
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i)
            HandleMotionEvent(this, &inputBuffer->motionEvents[i]);

        // Key events (back button, volume keys, gamepad, etc.)
        for (uint64_t i = 0; i < inputBuffer->keyEventsCount; ++i)
            HandleKeyEvent(this, &inputBuffer->keyEvents[i]);

        android_app_clear_motion_events(inputBuffer);
        android_app_clear_key_events(inputBuffer);
    }

    glm::vec2 AndroidWindow::GetSize() const
    {
        if (mNativeWindow)
            return { static_cast<float>(ANativeWindow_getWidth(mNativeWindow)), static_cast<float>(ANativeWindow_getHeight(mNativeWindow)) };

        return { static_cast<float>(mWindowData.Width), static_cast<float>(mWindowData.Height) };
    }

    void AndroidWindow::SetSize(const glm::vec2&) const
    {
        // ANativeWindow dimensions are display-controlled
    }

    // Lifecycle command callback (called on the main thread by the glue)
    void AndroidWindow::HandleAppCmd(android_app* app, int32_t cmd)
    {
        auto* self = static_cast<AndroidWindow*>(app->userData);
        if (!self)
            return;

        switch (cmd)
        {
        case APP_CMD_SAVE_STATE:
            Log<Severity::Info>("APP_CMD_SAVE_STATE");
            break;

        case APP_CMD_INIT_WINDOW:
        {
            const bool hadWindow = self->mNativeWindow != nullptr;
            self->mNativeWindow = app->window;
            if (self->mNativeWindow)
            {
                Uint w = static_cast<Uint>(ANativeWindow_getWidth(self->mNativeWindow));
                Uint h = static_cast<Uint>(ANativeWindow_getHeight(self->mNativeWindow));
                self->mWindowData.Width = w;
                self->mWindowData.Height = h;

                // A new ANativeWindow after TERM_WINDOW: the renderer has to build a new surface + swapchain for it
                if (!hadWindow && self->mEventCallback)
                {
                    WindowSurfaceCreatedEvent surfaceEvent(w, h);
                    self->mEventCallback(surfaceEvent);
                }

                WindowResizeEvent e(w, h);
                if (self->mEventCallback)
                    self->mEventCallback(e);
            }
            Log<Severity::Info>("APP_CMD_INIT_WINDOW ({}x{})", self->mWindowData.Width, self->mWindowData.Height);
            break;
        }

        case APP_CMD_TERM_WINDOW:
        {
            Log<Severity::Info>("APP_CMD_TERM_WINDOW");
            self->ReleaseAllPointers();
            // The window is still valid during this callback, release everything that references it before returning
            if (self->mNativeWindow && self->mEventCallback)
            {
                WindowSurfaceDestroyedEvent e;
                self->mEventCallback(e);
            }
            self->mNativeWindow = nullptr;
            break;
        }

        case APP_CMD_RESUME:
            Log<Severity::Info>("APP_CMD_RESUME");
            self->mResumed = true;
            break;

        case APP_CMD_PAUSE:
        {
            Log<Severity::Info>("APP_CMD_PAUSE");
            self->mResumed = false;
            self->ReleaseAllPointers();
            WindowLostFocusEvent e;
            self->Emit(e);
            break;
        }

        case APP_CMD_GAINED_FOCUS:
            Log<Severity::Info>("APP_CMD_GAINED_FOCUS");
            break;

        case APP_CMD_LOST_FOCUS:
        {
            // Still visible (notification shade, dialogs, multi-window), keep rendering; PAUSE/TERM_WINDOW stop us
            // but whatever was held (fingers, keys) will never see its release
            Log<Severity::Info>("APP_CMD_LOST_FOCUS");
            self->ReleaseAllPointers();
            WindowLostFocusEvent e;
            self->Emit(e);
            break;
        }

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
        {
            Log<Severity::Info>("APP_CMD_CONFIG_CHANGED");
            if (app->window)
            {
                Uint w = static_cast<Uint>(ANativeWindow_getWidth(app->window));
                Uint h = static_cast<Uint>(ANativeWindow_getHeight(app->window));
                self->mWindowData.Width = w;
                self->mWindowData.Height = h;

                WindowResizeEvent e(w, h);
                if (self->mEventCallback)
                    self->mEventCallback(e);
            }
            break;
        }

        case APP_CMD_SOFTWARE_KB_VIS_CHANGED:
            Android::OnSoftKeyboardVisibilityChanged(app->softwareKeyboardVisible);
            break;

        case APP_CMD_EDITOR_ACTION:
            Android::OnEditorAction(app->editorAction);
            break;

        case APP_CMD_DESTROY:
            Log<Severity::Info>("APP_CMD_DESTROY");
            // Actual event is fired via destroyRequested in Update()
            // To avoid double-firing. Nothing to do here
            break;

        default:
            break;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Input
    //   Touch screen : every pointer -> Touch*Event (multi touch). The primary (first) pointer also drives the emulated
    //                  mouse (MouseMoved/MouseButton events flagged fromTouch) and ImGui, so mouse-only code keeps working
    //   Mouse        : USB/Bluetooth mouse, DeX, ChromeOS -> real mouse events (left/right/middle, hover, wheel)
    //   Keyboard     : hardware keys -> Key*Event with the same codes as Windows, text -> KeyTypedEvent
    //   Gamepad      : buttons (key events) + sticks/triggers/d-pad (joystick motion events) -> InputState gamepads
    //   Back button  : Key::Escape, closes the app unless Input::SetBackButtonExitsApp(false)
    // ---------------------------------------------------------------------------------------------------------------

    static ImGuiIO* GetImGuiIO()
    {
        return Core::GetClient()->GetClientOptions().EnableImGui ? &ImGui::GetIO() : nullptr;
    }

    void AndroidWindow::HandleMotionEvent(AndroidWindow* window, const GameActivityMotionEvent* event)
    {
        if (!window || !event || event->pointerCount == 0)
            return;

        const int32_t source = event->source;
        if ((source & AINPUT_SOURCE_CLASS_JOYSTICK) == AINPUT_SOURCE_CLASS_JOYSTICK)
        {
            window->HandleGamepadMotion(event);
            return;
        }

        const int32_t action = event->action;
        const int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;

        // For POINTER_DOWN / POINTER_UP the acting pointer index is encoded in the action
        Uint pointerIndex = 0;
        if (actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN || actionMasked == AMOTION_EVENT_ACTION_POINTER_UP)
        {
            pointerIndex = static_cast<Uint>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            if (pointerIndex >= event->pointerCount)
                pointerIndex = 0;
        }

        const int32_t toolType = GameActivityPointerAxes_getToolType(&event->pointers[pointerIndex]);
        const bool isMouse = toolType == AMOTION_EVENT_TOOL_TYPE_MOUSE || ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE && toolType != AMOTION_EVENT_TOOL_TYPE_FINGER);
        if (isMouse)
            window->HandleMouseMotion(event, actionMasked, pointerIndex);
        else if ((source & AINPUT_SOURCE_CLASS_POINTER) == AINPUT_SOURCE_CLASS_POINTER)
            window->HandleTouchMotion(event, actionMasked, pointerIndex);
    }

    void AndroidWindow::HandleTouchMotion(const GameActivityMotionEvent* event, int32_t actionMasked, Uint pointerIndex)
    {
        ImGuiIO* io = GetImGuiIO();

        auto findPointer = [this](int32_t id) -> PointerState* {
            for (Uint i = 0; i < mPointerCount; i++)
                if (mPointers[i].ID == id)
                    return &mPointers[i];
            return nullptr;
        };
        auto removePointer = [this](int32_t id) {
            for (Uint i = 0; i < mPointerCount; i++)
            {
                if (mPointers[i].ID == id)
                {
                    mPointers[i] = mPointers[--mPointerCount];
                    return;
                }
            }
        };

        switch (actionMasked)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {
                const GameActivityPointerAxes& p = event->pointers[pointerIndex];
                const float x = GameActivityPointerAxes_getX(&p);
                const float y = GameActivityPointerAxes_getY(&p);
                if (!findPointer(p.id) && mPointerCount < MAX_POINTERS)
                    mPointers[mPointerCount++] = { p.id, x, y };

                const float pressure = GameActivityPointerAxes_getPressure(&p);
                TouchPressedEvent touchEvent(p.id, x, y, pressure > 0.0f ? pressure : 1.0f);
                Emit(touchEvent);

                // First finger down becomes the primary pointer = the emulated mouse
                if (mPrimaryPointer < 0)
                {
                    mPrimaryPointer = p.id;
                    MouseMovedEvent moveEvent(x, y, true);
                    MouseButtonPressedEvent pressEvent(x, y, static_cast<MouseCode>(Mouse::ButtonLeft), true);
                    Emit(moveEvent);
                    Emit(pressEvent);
                    if (io)
                    {
                        io->AddMousePosEvent(x, y);
                        io->AddMouseButtonEvent(0, true);
                    }
                }
                break;
            }

            case AMOTION_EVENT_ACTION_MOVE:
            {
                // MOVE carries every active pointer, only report the ones that actually moved
                for (Uint i = 0; i < event->pointerCount; ++i)
                {
                    const GameActivityPointerAxes& p = event->pointers[i];
                    const float x = GameActivityPointerAxes_getX(&p);
                    const float y = GameActivityPointerAxes_getY(&p);
                    PointerState* state = findPointer(p.id);
                    if (state && state->X == x && state->Y == y)
                        continue;

                    if (state)
                    {
                        state->X = x;
                        state->Y = y;
                    }

                    const float pressure = GameActivityPointerAxes_getPressure(&p);
                    TouchMovedEvent touchEvent(p.id, x, y, pressure > 0.0f ? pressure : 1.0f);
                    Emit(touchEvent);

                    if (p.id == mPrimaryPointer)
                    {
                        MouseMovedEvent moveEvent(x, y, true);
                        Emit(moveEvent);
                        if (io)
                            io->AddMousePosEvent(x, y);
                    }
                }
                break;
            }

            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
            {
                const GameActivityPointerAxes& p = event->pointers[pointerIndex];
                const float x = GameActivityPointerAxes_getX(&p);
                const float y = GameActivityPointerAxes_getY(&p);
                removePointer(p.id);

                TouchReleasedEvent touchEvent(p.id, x, y, (event->flags & MOTION_EVENT_FLAG_CANCELED) != 0);
                Emit(touchEvent);

                if (p.id == mPrimaryPointer)
                {
                    mPrimaryPointer = -1;
                    MouseButtonReleasedEvent releaseEvent(x, y, static_cast<MouseCode>(Mouse::ButtonLeft), true);
                    Emit(releaseEvent);
                    if (io)
                        io->AddMouseButtonEvent(0, false);
                }
                break;
            }

            case AMOTION_EVENT_ACTION_CANCEL:
                ReleaseAllPointers();
                break;

            default:
                break;
        }
    }

    void AndroidWindow::HandleMouseMotion(const GameActivityMotionEvent* event, int32_t actionMasked, uint32_t pointerIndex)
    {
        ImGuiIO* io = GetImGuiIO();
        const GameActivityPointerAxes& p = event->pointers[pointerIndex];
        const float x = GameActivityPointerAxes_getX(&p);
        const float y = GameActivityPointerAxes_getY(&p);

        if (actionMasked == AMOTION_EVENT_ACTION_SCROLL)
        {
            const float wheel = GameActivityPointerAxes_getAxisValue(&p, AMOTION_EVENT_AXIS_VSCROLL);
            if (wheel != 0.0f)
            {
                MouseScrolledEvent scrollEvent(wheel);
                Emit(scrollEvent);
            }
            if (io)
                io->AddMouseWheelEvent(GameActivityPointerAxes_getAxisValue(&p, AMOTION_EVENT_AXIS_HSCROLL), wheel);
            return;
        }

        if (x != mMouseX || y != mMouseY)
        {
            mMouseX = x;
            mMouseY = y;
            MouseMovedEvent moveEvent(x, y);
            Emit(moveEvent);
            if (io)
                io->AddMousePosEvent(x, y);
        }

        // Android button bits match Surge's Mouse codes: PRIMARY = 1 (left), SECONDARY = 2 (right), TERTIARY = 4 (middle)
        int32_t buttons = event->buttonState & (AMOTION_EVENT_BUTTON_PRIMARY | AMOTION_EVENT_BUTTON_SECONDARY | AMOTION_EVENT_BUTTON_TERTIARY);
        if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_CANCEL)
            buttons = 0;

        const int32_t changed = buttons ^ mMouseButtons;
        for (int32_t bit : { AMOTION_EVENT_BUTTON_PRIMARY, AMOTION_EVENT_BUTTON_SECONDARY, AMOTION_EVENT_BUTTON_TERTIARY })
        {
            if (!(changed & bit))
                continue;

            const MouseCode code = static_cast<MouseCode>(bit);
            const int imguiButton = (bit == AMOTION_EVENT_BUTTON_PRIMARY) ? 0 : (bit == AMOTION_EVENT_BUTTON_SECONDARY ? 1 : 2);
            if (buttons & bit)
            {
                MouseButtonPressedEvent pressEvent(x, y, code);
                Emit(pressEvent);
            }
            else
            {
                MouseButtonReleasedEvent releaseEvent(x, y, code);
                Emit(releaseEvent);
            }
            if (io)
                io->AddMouseButtonEvent(imguiButton, (buttons & bit) != 0);
        }
        mMouseButtons = buttons;
    }

    void AndroidWindow::ReleaseAllPointers()
    {
        for (uint32_t i = 0; i < mPointerCount; i++)
        {
            TouchReleasedEvent touchEvent(mPointers[i].ID, mPointers[i].X, mPointers[i].Y, true);
            Emit(touchEvent);
            if (mPointers[i].ID == mPrimaryPointer)
            {
                MouseButtonReleasedEvent releaseEvent(mPointers[i].X, mPointers[i].Y, static_cast<MouseCode>(Mouse::ButtonLeft), true);
                Emit(releaseEvent);
            }
        }
        mPointerCount = 0;
        mPrimaryPointer = -1;

        for (int32_t bit : { AMOTION_EVENT_BUTTON_PRIMARY, AMOTION_EVENT_BUTTON_SECONDARY, AMOTION_EVENT_BUTTON_TERTIARY })
        {
            if (mMouseButtons & bit)
            {
                MouseButtonReleasedEvent releaseEvent(mMouseX, mMouseY, static_cast<MouseCode>(bit));
                Emit(releaseEvent);
            }
        }
        mMouseButtons = 0;

        if (ImGuiIO* io = GetImGuiIO())
            for (int b = 0; b < 3; b++)
                io->AddMouseButtonEvent(b, false);
    }

    // Gamepads ---------------------------------------------------------------------------------------------------------
    int AndroidWindow::GamepadSlot(int32_t deviceId)
    {
        for (int i = 0; i < 4; i++)
            if (mGamepadDevices[i] == deviceId)
                return i;
        for (int i = 0; i < 4; i++)
        {
            if (mGamepadDevices[i] < 0)
            {
                mGamepadDevices[i] = deviceId;
                Log<Severity::Info>("[Input] Gamepad connected (device {}) -> slot {}", deviceId, i);
                return i;
            }
        }
        return -1;
    }

    void AndroidWindow::HandleGamepadMotion(const GameActivityMotionEvent* event)
    {
        const int slot = GamepadSlot(event->deviceId);
        if (slot < 0)
            return;

        const GameActivityPointerAxes* p = &event->pointers[0];
        const Uint index = static_cast<Uint>(slot);
        auto axis = [p](int32_t a) { return GameActivityPointerAxes_getAxisValue(p, a); };

        // Android sticks are already +Y down
        InputState::SetGamepadAxis(index, GamepadAxis::LeftX, axis(AMOTION_EVENT_AXIS_X));
        InputState::SetGamepadAxis(index, GamepadAxis::LeftY, axis(AMOTION_EVENT_AXIS_Y));

        // Most controllers report the right stick on Z/RZ, some on RX/RY
        float rx = axis(AMOTION_EVENT_AXIS_Z), ry = axis(AMOTION_EVENT_AXIS_RZ);
        if (rx == 0.0f && ry == 0.0f)
        {
            rx = axis(AMOTION_EVENT_AXIS_RX);
            ry = axis(AMOTION_EVENT_AXIS_RY);
        }
        InputState::SetGamepadAxis(index, GamepadAxis::RightX, rx);
        InputState::SetGamepadAxis(index, GamepadAxis::RightY, ry);
        InputState::SetGamepadAxis(index, GamepadAxis::LeftTrigger, std::max(axis(AMOTION_EVENT_AXIS_LTRIGGER), axis(AMOTION_EVENT_AXIS_BRAKE)));
        InputState::SetGamepadAxis(index, GamepadAxis::RightTrigger, std::max(axis(AMOTION_EVENT_AXIS_RTRIGGER), axis(AMOTION_EVENT_AXIS_GAS)));

        // D-pad as a hat (controllers that don't send DPAD key events)
        const float hatX = axis(AMOTION_EVENT_AXIS_HAT_X);
        const float hatY = axis(AMOTION_EVENT_AXIS_HAT_Y);
        if (hatX != mGamepadHat[slot][0] || hatY != mGamepadHat[slot][1])
        {
            InputState::SetGamepadButton(index, GamepadButton::DPadLeft, hatX < -0.5f);
            InputState::SetGamepadButton(index, GamepadButton::DPadRight, hatX > 0.5f);
            InputState::SetGamepadButton(index, GamepadButton::DPadUp, hatY < -0.5f);
            InputState::SetGamepadButton(index, GamepadButton::DPadDown, hatY > 0.5f);
            mGamepadHat[slot][0] = hatX;
            mGamepadHat[slot][1] = hatY;
        }
    }

    static bool TranslateGamepadKey(int32_t keyCode, GamepadButton& out)
    {
        switch (keyCode)
        {
            case AKEYCODE_BUTTON_A: out = GamepadButton::A; return true;
            case AKEYCODE_BUTTON_B: out = GamepadButton::B; return true;
            case AKEYCODE_BUTTON_X: out = GamepadButton::X; return true;
            case AKEYCODE_BUTTON_Y: out = GamepadButton::Y; return true;
            case AKEYCODE_BUTTON_L1: out = GamepadButton::LeftShoulder; return true;
            case AKEYCODE_BUTTON_R1: out = GamepadButton::RightShoulder; return true;
            case AKEYCODE_BUTTON_SELECT: out = GamepadButton::Back; return true;
            case AKEYCODE_BUTTON_START: out = GamepadButton::Start; return true;
            case AKEYCODE_BUTTON_MODE: out = GamepadButton::Guide; return true;
            case AKEYCODE_BUTTON_THUMBL: out = GamepadButton::LeftThumb; return true;
            case AKEYCODE_BUTTON_THUMBR: out = GamepadButton::RightThumb; return true;
            case AKEYCODE_DPAD_UP: out = GamepadButton::DPadUp; return true;
            case AKEYCODE_DPAD_DOWN: out = GamepadButton::DPadDown; return true;
            case AKEYCODE_DPAD_LEFT: out = GamepadButton::DPadLeft; return true;
            case AKEYCODE_DPAD_RIGHT: out = GamepadButton::DPadRight; return true;
            default: return false;
        }
    }

    // Android key codes -> Surge::Key (Windows virtual key codes), so a key has the same code on both platforms
    static KeyCode TranslateKey(int32_t keyCode)
    {
        if (keyCode >= AKEYCODE_A && keyCode <= AKEYCODE_Z)
            return static_cast<KeyCode>('A' + (keyCode - AKEYCODE_A));
        if (keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9)
            return static_cast<KeyCode>('0' + (keyCode - AKEYCODE_0));
        if (keyCode >= AKEYCODE_F1 && keyCode <= AKEYCODE_F12)
            return static_cast<KeyCode>(static_cast<KeyCode>(Key::F1) + (keyCode - AKEYCODE_F1));
        if (keyCode >= AKEYCODE_NUMPAD_0 && keyCode <= AKEYCODE_NUMPAD_9)
            return static_cast<KeyCode>(static_cast<KeyCode>(Key::KP0) + (keyCode - AKEYCODE_NUMPAD_0));

        switch (keyCode)
        {
            case AKEYCODE_SPACE: return static_cast<KeyCode>(Key::Space);
            case AKEYCODE_ENTER:
            case AKEYCODE_NUMPAD_ENTER: return static_cast<KeyCode>(Key::Enter);
            case AKEYCODE_DEL: return static_cast<KeyCode>(Key::Backspace);
            case AKEYCODE_FORWARD_DEL: return static_cast<KeyCode>(Key::Delete);
            case AKEYCODE_TAB: return static_cast<KeyCode>(Key::Tab);
            case AKEYCODE_ESCAPE:
            case AKEYCODE_BACK: return static_cast<KeyCode>(Key::Escape);
            case AKEYCODE_DPAD_UP: return static_cast<KeyCode>(Key::Up);
            case AKEYCODE_DPAD_DOWN: return static_cast<KeyCode>(Key::Down);
            case AKEYCODE_DPAD_LEFT: return static_cast<KeyCode>(Key::Left);
            case AKEYCODE_DPAD_RIGHT: return static_cast<KeyCode>(Key::Right);
            case AKEYCODE_SHIFT_LEFT: return static_cast<KeyCode>(Key::LeftShift);
            case AKEYCODE_SHIFT_RIGHT: return static_cast<KeyCode>(Key::RightShift);
            case AKEYCODE_CTRL_LEFT: return static_cast<KeyCode>(Key::LeftControl);
            case AKEYCODE_CTRL_RIGHT: return static_cast<KeyCode>(Key::RightControl);
            case AKEYCODE_ALT_LEFT: return static_cast<KeyCode>(Key::LeftAlt);
            case AKEYCODE_ALT_RIGHT: return static_cast<KeyCode>(Key::RightAlt);
            case AKEYCODE_COMMA: return static_cast<KeyCode>(Key::Comma);
            case AKEYCODE_PERIOD: return static_cast<KeyCode>(Key::Period);
            case AKEYCODE_MINUS: return static_cast<KeyCode>(Key::Minus);
            case AKEYCODE_SEMICOLON: return static_cast<KeyCode>(Key::Semicolon);
            case AKEYCODE_LEFT_BRACKET: return static_cast<KeyCode>(Key::LeftBracket);
            case AKEYCODE_RIGHT_BRACKET: return static_cast<KeyCode>(Key::RightBracket);
            case AKEYCODE_BACKSLASH: return static_cast<KeyCode>(Key::BackSlash);
            case AKEYCODE_APOSTROPHE: return static_cast<KeyCode>(Key::GraveAccent); // VK_OEM_7, what Windows reports for this key // ??
            case AKEYCODE_GRAVE: return 192;      // VK_OEM_3
            case AKEYCODE_EQUALS:
            case AKEYCODE_PLUS: return 187;       // VK_OEM_PLUS
            case AKEYCODE_SLASH: return 191;      // VK_OEM_2
            case AKEYCODE_PAGE_UP: return static_cast<KeyCode>(Key::PageUp);
            case AKEYCODE_PAGE_DOWN: return static_cast<KeyCode>(Key::PageDown);
            case AKEYCODE_MOVE_HOME: return static_cast<KeyCode>(Key::Home);
            case AKEYCODE_MOVE_END: return static_cast<KeyCode>(Key::End);
            case AKEYCODE_INSERT: return static_cast<KeyCode>(Key::Insert);
            case AKEYCODE_CAPS_LOCK: return static_cast<KeyCode>(Key::CapsLock);
            case AKEYCODE_NUM_LOCK: return static_cast<KeyCode>(Key::NumLock);
            case AKEYCODE_SCROLL_LOCK: return static_cast<KeyCode>(Key::ScrollLock);
            case AKEYCODE_BREAK: return static_cast<KeyCode>(Key::Pause);
            case AKEYCODE_SYSRQ: return static_cast<KeyCode>(Key::PrintScreen);
            case AKEYCODE_NUMPAD_MULTIPLY: return static_cast<KeyCode>(Key::KPMultiply);
            case AKEYCODE_NUMPAD_ADD: return static_cast<KeyCode>(Key::KPAdd);
            case AKEYCODE_NUMPAD_SUBTRACT: return static_cast<KeyCode>(Key::KPSubtract);
            case AKEYCODE_NUMPAD_DOT: return static_cast<KeyCode>(Key::KPDecimal);
            case AKEYCODE_NUMPAD_DIVIDE: return static_cast<KeyCode>(Key::KPDivide);
            case AKEYCODE_NUMPAD_EQUALS: return static_cast<KeyCode>(Key::KPEqual);
            default:
                return 0;
        }
    }

    void AndroidWindow::HandleKeyEvent(AndroidWindow* window, const GameActivityKeyEvent* event)
    {
        if (!window || !event)
            return;

        const bool down = (event->action == AKEY_EVENT_ACTION_DOWN);
        const bool up = (event->action == AKEY_EVENT_ACTION_UP);
        if (!down && !up)
            return;

        // Controller buttons (and d-pads of controllers) go to the gamepad state, not the keyboard
        const int32_t source = event->source;
        const bool fromGamepad = (source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD || (source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK;
        GamepadButton button;
        if (TranslateGamepadKey(event->keyCode, button) && (fromGamepad || (event->keyCode >= AKEYCODE_BUTTON_A && event->keyCode <= AKEYCODE_BUTTON_MODE)))
        {
            const int slot = window->GamepadSlot(event->deviceId);
            if (slot >= 0)
                InputState::SetGamepadButton(static_cast<Uint>(slot), button, down);
            return;
        }
        if (fromGamepad && (event->keyCode == AKEYCODE_BUTTON_L2 || event->keyCode == AKEYCODE_BUTTON_R2))
        {
            // Digital triggers
            const int slot = window->GamepadSlot(event->deviceId);
            if (slot >= 0)
                InputState::SetGamepadAxis(static_cast<Uint>(slot), event->keyCode == AKEYCODE_BUTTON_L2 ? GamepadAxis::LeftTrigger : GamepadAxis::RightTrigger, down ? 1.0f : 0.0f);
            return;
        }

        const KeyCode key = TranslateKey(event->keyCode);
        bool pressHandled = false;
        if (key != 0)
        {
            if (down)
            {
                KeyPressedEvent pressEvent(static_cast<Key>(key), static_cast<uint16_t>(event->repeatCount));
                window->Emit(pressEvent);
                pressHandled = pressEvent.Handled;
            }
            else
            {
                KeyReleasedEvent releaseEvent(key);
                window->Emit(releaseEvent);
            }
        }

        if (down && event->unicodeChar > 0 && event->unicodeChar < 0x10000)
        {
            KeyTypedEvent typedEvent(static_cast<KeyCode>(event->unicodeChar));
            window->Emit(typedEvent);
            if (ImGuiIO* io = GetImGuiIO())
                io->AddInputCharacter(static_cast<unsigned int>(event->unicodeChar));
        }

        // Back button: closes the app unless the game handles it (Input::SetBackButtonExitsApp(false))
        // A Back press the UI consumed (closing a popup, leaving a text field) never exits
        if (event->keyCode == AKEYCODE_BACK && down && event->repeatCount == 0)
            window->mBackExitsOnRelease = Input::DoesBackButtonExitApp() && !pressHandled;
        if (event->keyCode == AKEYCODE_BACK && up && window->mBackExitsOnRelease)
        {
            window->mBackExitsOnRelease = false;
            // TODO (Rid): Have a proper way to store and manage states in Android
            // Currently we destroy the window and be done with it
            WindowClosedEvent e;
            window->Emit(e);
        }
    }

} // namespace Surge

#endif // SURGE_PLATFORM_ANDROID