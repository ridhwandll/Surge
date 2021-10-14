// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class Titlebar
    {
    public:
        Titlebar() = default;
        ~Titlebar() = default;

        void Render();
        float GetHeight() { return 60.0f; }
    };

} // namespace Surge
