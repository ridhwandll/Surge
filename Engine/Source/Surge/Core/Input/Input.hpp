// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Input/KeyCodes.hpp"
#include "Surge/Core/Input/MouseCodes.hpp"
#include <glm/vec2.hpp>

namespace Surge
{
    enum class CursorMode
    {
        Normal = 0,
        Locked
    };

    // A touch reports BEGAN in the frame it started and ENDED/CANCELED in the frame it ended (even if both happened
    // between two frames), it is removed in the frame after that
    enum class TouchPhase : uint8_t
    {
        BEGAN = 0,
        MOVED,
        STATIONARY,
        ENDED,
        CANCELED // The OS took the contact away (gesture, focus loss): not a release, don't treat it as a tap
    };

    struct Touch
    {
        int32_t ID = -1;                         // Stable while the contact lasts, the smallest free index (0, 1, 2...)
        TouchPhase Phase = TouchPhase::BEGAN;
        glm::vec2 Position = { 0.0f, 0.0f };      // Window client pixels, the same space as mouse events and the UI system
        glm::vec2 StartPosition = { 0.0f, 0.0f };
        glm::vec2 Delta = { 0.0f, 0.0f };         // Movement since the previous frame
        float Pressure = 1.0f;
        float Duration = 0.0f;                   // Seconds since the touch began
        bool IsMouse = false;                    // Emulated by the left mouse button (desktop / mouse on Android)
    };

    // Xbox layout names, A = bottom face button
    enum class GamepadButton : uint8_t
    {
        A = 0, B, X, Y,
        LeftShoulder, RightShoulder,
        Back, Start, Guide,
        LeftThumb, RightThumb,
        DPadUp, DPadDown, DPadLeft, DPadRight,
        COUNT
    };

    // Sticks are -1..1 with +X = right and +Y = down (screen space, same as touch deltas), triggers are 0..1
    enum class GamepadAxis : uint8_t
    {
        LeftX = 0, LeftY,
        RightX, RightY,
        LeftTrigger, RightTrigger,
        COUNT
    };

    class Input
    {
    public:
        static constexpr Uint MAX_GAMEPADS = 4;

        // Keyboard
        static bool IsKeyPressed(Key key);   // Held
        static bool WasKeyPressed(Key key);  // Went down this frame
        static bool WasKeyReleased(Key key); // Went up this frame

        // Mouse (on touch devices the primary touch also drives the left button and the position)
        static bool IsMouseButtonPressed(Mouse button);
        static bool WasMouseButtonPressed(Mouse button);
        static bool WasMouseButtonReleased(Mouse button);
        static Pair<float, float> GetMousePosition(); // Window client pixels
        static float GetMouseX();
        static float GetMouseY();
        static float GetMouseScrollDelta(); // Wheel notches this frame, + = away from the user
        static void SetCursorMode(CursorMode cursorMode);

        // Touch (multi touch on Android and Windows touch screens, the left mouse button is touch "IsMouse" when enabled)
        static Uint GetTouchCount();
        static const Touch& GetTouch(Uint index); // 0 <= index < GetTouchCount(), ordered by start time
        static const Touch* FindTouch(int32_t id); // nullptr if no touch has this ID this frame
        static bool IsTouchSupported();            // The device has a touch screen
        static void SetMouseTouchEmulation(bool enabled);
        static bool IsMouseTouchEmulationEnabled();

        // Gamepads (XInput on Windows, Bluetooth/USB controllers on Android), index 0 to MAX_GAMEPADS - 1
        static bool IsGamepadConnected(Uint index);
        static bool IsGamepadButtonPressed(Uint index, GamepadButton button);
        static bool WasGamepadButtonPressed(Uint index, GamepadButton button);
        static bool WasGamepadButtonReleased(Uint index, GamepadButton button);
        static float GetGamepadAxis(Uint index, GamepadAxis axis); // With deadzones applied

        // Android back button: always reported as Key::Escape, by default it also closes the app
        static void SetBackButtonExitsApp(bool exits);
        static bool DoesBackButtonExitApp();
    };

} // namespace Surge
