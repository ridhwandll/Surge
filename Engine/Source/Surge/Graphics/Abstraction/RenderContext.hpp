// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    // NOTE(Rid):
    // The "Core" owns the RenderContext
    // RenderContext owns the API Instance, LogicalDevice, SwapChain and PhysicalDevice

    struct GPUMemoryStats
    {
        GPUMemoryStats() = default;
        GPUMemoryStats(uint64_t used, uint64_t free)
            : Used(used), Free(free) {}

        uint64_t Used = 0;
        uint64_t Free = 0;
    };

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

        virtual GPUMemoryStats GetMemoryStatus() const = 0;

        static Scope<RenderContext> Create();
    };
}
