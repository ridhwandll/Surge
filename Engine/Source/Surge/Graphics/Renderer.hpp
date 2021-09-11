// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/RendererBase.hpp"

namespace Surge
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize();

        void RenderDatDamnTriangle(); // TODO: Remove

        void Shutdown();
    private:
        RendererBase mBase;
    };
}
