// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Input/Input.hpp"
#include "Surge/Core/Input/InputState.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Window/Window.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>

namespace Surge
{
    static bool IsSurgeWindowActive()
    {
        return GetActiveWindow() == Surge::Core::GetWindow()->GetNativeWindowHandle();
    }

    bool Input::IsKeyPressed(Key key)
    {
        if (IsSurgeWindowActive())
        {
            SHORT state = GetAsyncKeyState(static_cast<int>(key));
            return (state & 0x8000);
        }
        return false;
    }

    bool Input::IsMouseButtonPressed(Mouse button)
    {
        if (IsSurgeWindowActive())
        {
            SHORT state = GetAsyncKeyState(static_cast<int>(button));
            return (state & 0x8000);
        }
        return false;
    }

    // Client coordinates of the Surge window: the same space as mouse/touch events and the UI system
    Pair<float, float> Input::GetMousePosition()
    {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(static_cast<HWND>(Core::GetWindow()->GetNativeWindowHandle()), &p);
        return {(float)p.x, (float)p.y};
    }

    float Input::GetMouseX() { return GetMousePosition().Data1; }

    float Input::GetMouseY() { return GetMousePosition().Data2; }

    void Input::SetCursorMode(CursorMode cursorMode)
    {
        static CursorMode sLastCursorMode = CursorMode::Normal;
        if(sLastCursorMode == cursorMode)
            return;

        HWND hWnd = (HWND)Core::GetWindow()->GetNativeWindowHandle();

        if(cursorMode == CursorMode::Normal)
        {
            ClipCursor(NULL);
            ShowCursor(TRUE);
        }
        else if(cursorMode == CursorMode::Locked)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);

            POINT ul = { rect.left, rect.top };
            POINT lr = { rect.right, rect.bottom };
            ClientToScreen(hWnd, &ul);
            ClientToScreen(hWnd, &lr);

            RECT screenRect = { ul.x, ul.y, lr.x, lr.y };

            ClipCursor(&screenRect);
            ShowCursor(FALSE);
        }

        sLastCursorMode = cursorMode;
    }

    // XInput gamepads. The dll is loaded at runtime (no import lib needed, missing on very old systems = no gamepads).
    // Polling a disconnected slot is slow, so empty slots are only probed every PROBE_INTERVAL frames
    namespace
    {
        using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
        constexpr Uint PROBE_INTERVAL = 120;

        struct XInputData
        {
            bool Loaded = false;
            XInputGetStateFn GetState = nullptr;
            bool Connected[Input::MAX_GAMEPADS] = {};
            Uint ProbeCountdown = 0;
        };
        XInputData sXInput;

        void LoadXInput()
        {
            sXInput.Loaded = true;
            for (const char* dll : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" })
            {
                if (HMODULE module = LoadLibraryA(dll))
                {
                    sXInput.GetState = reinterpret_cast<XInputGetStateFn>(reinterpret_cast<void*>(GetProcAddress(module, "XInputGetState")));
                    if (sXInput.GetState)
                        return;
                }
            }
        }

        float StickAxis(SHORT value) { return value < 0 ? static_cast<float>(value) / 32768.0f : static_cast<float>(value) / 32767.0f; }

        void ApplyXInputState(Uint index, const XINPUT_GAMEPAD& pad, bool active)
        {
            // An inactive window must not react to the controller (same rule as the keyboard)
            const WORD buttons = active ? pad.wButtons : 0;
            struct Mapping { WORD Mask; GamepadButton Button; };
            static constexpr Mapping sMappings[] = {
                { XINPUT_GAMEPAD_A, GamepadButton::A }, { XINPUT_GAMEPAD_B, GamepadButton::B },
                { XINPUT_GAMEPAD_X, GamepadButton::X }, { XINPUT_GAMEPAD_Y, GamepadButton::Y },
                { XINPUT_GAMEPAD_LEFT_SHOULDER, GamepadButton::LeftShoulder }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, GamepadButton::RightShoulder },
                { XINPUT_GAMEPAD_BACK, GamepadButton::Back }, { XINPUT_GAMEPAD_START, GamepadButton::Start },
                { XINPUT_GAMEPAD_LEFT_THUMB, GamepadButton::LeftThumb }, { XINPUT_GAMEPAD_RIGHT_THUMB, GamepadButton::RightThumb },
                { XINPUT_GAMEPAD_DPAD_UP, GamepadButton::DPadUp }, { XINPUT_GAMEPAD_DPAD_DOWN, GamepadButton::DPadDown },
                { XINPUT_GAMEPAD_DPAD_LEFT, GamepadButton::DPadLeft }, { XINPUT_GAMEPAD_DPAD_RIGHT, GamepadButton::DPadRight },
            };
            for (const Mapping& m : sMappings)
                InputState::SetGamepadButton(index, m.Button, (buttons & m.Mask) != 0);

            // XInput sticks are +Y up, Surge uses +Y down (screen space)
            InputState::SetGamepadAxis(index, GamepadAxis::LeftX, active ? StickAxis(pad.sThumbLX) : 0.0f);
            InputState::SetGamepadAxis(index, GamepadAxis::LeftY, active ? -StickAxis(pad.sThumbLY) : 0.0f);
            InputState::SetGamepadAxis(index, GamepadAxis::RightX, active ? StickAxis(pad.sThumbRX) : 0.0f);
            InputState::SetGamepadAxis(index, GamepadAxis::RightY, active ? -StickAxis(pad.sThumbRY) : 0.0f);
            InputState::SetGamepadAxis(index, GamepadAxis::LeftTrigger, active ? static_cast<float>(pad.bLeftTrigger) / 255.0f : 0.0f);
            InputState::SetGamepadAxis(index, GamepadAxis::RightTrigger, active ? static_cast<float>(pad.bRightTrigger) / 255.0f : 0.0f);
        }
    }

    void InputState::PlatformBeginFrame()
    {
        static bool sTouchChecked = false;
        if (!sTouchChecked)
        {
            sTouchChecked = true;
            InputState::SetTouchSupported(GetSystemMetrics(SM_MAXIMUMTOUCHES) > 0);
        }

        if (!sXInput.Loaded)
            LoadXInput();
        if (!sXInput.GetState)
            return;

        const bool probe = (sXInput.ProbeCountdown == 0);
        sXInput.ProbeCountdown = probe ? PROBE_INTERVAL : sXInput.ProbeCountdown - 1;
        const bool active = IsSurgeWindowActive();

        for (Uint i = 0; i < Input::MAX_GAMEPADS; i++)
        {
            if (!sXInput.Connected[i] && !probe)
                continue;

            XINPUT_STATE state = {};
            const bool connected = (sXInput.GetState(i, &state) == static_cast<DWORD>(ERROR_SUCCESS));
            if (connected)
                ApplyXInputState(i, state.Gamepad, active);
            else if (sXInput.Connected[i])
                InputState::SetGamepadConnected(i, false);
            sXInput.Connected[i] = connected;
        }
    }
} // namespace Surge
