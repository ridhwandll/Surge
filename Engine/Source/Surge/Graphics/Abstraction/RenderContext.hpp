// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    // NOTE(Rid):
    // A Window owns the RenderContext
    // RenderContext owns the LogicalDevice, SwapChain and PhysicalDevice

    class SURGE_API RenderContext
    {
    public:
        RenderContext() = default;
        virtual ~RenderContext() = default;

        virtual void Initialize(Window* window) = 0;
        virtual void Shutdown() = 0;

        static Scope<RenderContext> Create();
    };
}
