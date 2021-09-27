// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    // NOTE(Rid):
    // The "Core" owns the RenderContext
    // RenderContext owns the API Instance, LogicalDevice, SwapChain, PhysicalDevice, MemoryAllocator etc.

    struct GPUInfo
    {
        String Name;
        int64_t DeviceScore;
        //TODO: Add more stuff?
    };

    struct GPUMemoryStats
    {
        GPUMemoryStats() = default;
        GPUMemoryStats(uint64_t used, uint64_t free)
            : Used(used), Free(free) {}

        uint64_t Used = 0;
        uint64_t Free = 0;
    };

    class RenderContext
    {
    public:
        RenderContext() = default;
        virtual ~RenderContext() = default;

        virtual void Initialize(Window* window) = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void OnResize() = 0;
        virtual void RenderImGui() = 0;
        virtual Uint GetFrameIndex() const = 0;

        virtual GPUMemoryStats GetMemoryStatus() const = 0;
        virtual GPUInfo GetGPUInfo() const = 0;
        virtual void* GetMemoryAllocator() const = 0;

        static Scope<RenderContext> Create();
    };
}
