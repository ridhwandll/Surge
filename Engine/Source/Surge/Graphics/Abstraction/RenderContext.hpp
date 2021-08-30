// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    // NOTE(Rid):
    // The "Core" owns the RenderContext
    // RenderContext owns the API Instance, LogicalDevice, SwapChain and PhysicalDevice

    class SURGE_API RenderContext
    {
    public:
        RenderContext() = default;
        virtual ~RenderContext() = default;

        virtual void Initialize(Window* window) = 0;
        virtual void Shutdown() = 0;

        virtual void OnResize(Uint width, Uint height) = 0;

        virtual void* GetInteralDevice() = 0;
        virtual void* GetInteralInstance() = 0;

        static Scope<RenderContext> Create();
    };
}
