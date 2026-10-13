// Copyright (c) - SurgeTechnologies - All rights reserved
#ifdef SURGE_PLATFORM_ANDROID
#include "Surge/Core/Input/Input.hpp"
#include "Surge/Core/Input/InputState.hpp"

namespace Surge
{
    // Android can't be polled: keyboard, mouse and touch state is built from the events AndroidWindow emits.
    // The primary touch drives the emulated mouse (left button + position), a real mouse drives it too
    bool Input::IsKeyPressed(Key key) { return InputState::IsKeyDown(key); }

    bool Input::IsMouseButtonPressed(Mouse button) { return InputState::IsMouseButtonDown(button); }

    Pair<float, float> Input::GetMousePosition()
    {
        const glm::vec2 position = InputState::GetMousePosition();
        return {position.x, position.y};
    }

    float Input::GetMouseX() { return InputState::GetMousePosition().x; }

    float Input::GetMouseY() { return InputState::GetMousePosition().y; }

    void Input::SetCursorMode(CursorMode)
    {
        // No cursor to lock on Android
    }

    void Input::SetMousePosition(float, float)
    {
        // No cursor to move on Android
    }

    // Gamepads and touches are event driven on Android (AndroidWindow), nothing to poll
    void InputState::PlatformBeginFrame() {}

} // namespace Surge

#endif // SURGE_PLATFORM_ANDROID
