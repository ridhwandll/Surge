// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Input/Input.hpp"

namespace Surge
{
    class Event;
}

// Platform independent input state behind Surge::Input. Platform windows only emit events (keys, mouse, touches),
// gamepads are pushed by the platform layer. Core drives the frame bookkeeping:
//   Core::Run      -> InputState::BeginFrame(dt) before the window pumps its events
//   Core::OnEvent  -> InputState::OnEvent(e) before anything else sees the event
namespace Surge::InputState
{
    void BeginFrame(float deltaSeconds);
    void OnEvent(Event& e);

    // Held state as seen through events (used where the OS can't be polled, e.g. Android)
    bool IsKeyDown(Key key);
    bool IsMouseButtonDown(Mouse button);
    glm::vec2 GetMousePosition();

    // Gamepads, written by the platform layer (raw values, Input applies the deadzones)
    void SetGamepadConnected(Uint index, bool connected);
    void SetGamepadButton(Uint index, GamepadButton button, bool down);
    void SetGamepadAxis(Uint index, GamepadAxis axis, float value);

    void SetTouchSupported(bool supported);

    // Implemented per platform (WindowsInput.cpp / AndroidInput.cpp), called at the start of BeginFrame
    void PlatformBeginFrame();
}
