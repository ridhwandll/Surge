// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Texture.hpp"

namespace Surge
{
    class Editor;
    class Titlebar
    {
    public:
        Titlebar() = default;
        ~Titlebar() = default;

        void OnInit();
        void Render();
        float GetHeight() { return 60.0f; }

    private:
        Editor* mEditor;
        Ref<Texture2D> mIcon;
    };
} // namespace Surge