// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Input/Input.hpp"

namespace Surge
{
    bool Input::IsKeyPressed(KeyCode key)
    {
        SHORT state = GetAsyncKeyState(static_cast<int>(key));
        return (state & 0x8000);
    }

    bool Input::IsMouseButtonPressed(const MouseCode button)
    {
        SHORT state = GetAsyncKeyState(static_cast<int>(button));
        return (state & 0x8000);
    }

    Pair<float, float> Input::GetMousePosition()
    {
        POINT p;
        GetCursorPos(&p);
        return {(float)p.x, (float)p.y};
    }

    float Input::GetMouseX() { return GetMousePosition().Data1; }

    float Input::GetMouseY() { return GetMousePosition().Data2; }

    void Input::SetCursorMode(CursorMode cursorMode)
    {
        switch (cursorMode)
        {
            case CursorMode::Normal: SetCursor(LoadCursor(nullptr, IDC_ARROW)); break;
            case CursorMode::Locked: SetCursor(nullptr); break;
        }
    }
} // namespace Surge
