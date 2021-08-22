// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Core/KeyCodes.hpp"
#include "Core/MouseCodes.hpp"

namespace Surge
{
    enum class CursorMode
    {
        Normal = 0,
        Locked
    };

    class SURGE_API Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(const MouseCode button);

        static Pair<float, float> GetMousePosition();
        static float GetMousePositionX();
        static float GetMousePositionY();
        static void SetCursorMode(CursorMode cursorMode);
    };
}
