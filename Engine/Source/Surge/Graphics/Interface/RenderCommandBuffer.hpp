// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    class SURGE_API RenderCommandBuffer : public RefCounted
    {
    public:
        virtual ~RenderCommandBuffer() = default;

        virtual void BeginRecording() = 0;
        virtual void EndRecording() = 0;
        virtual void Submit() = 0;

        static Ref<RenderCommandBuffer> Create(bool createFromSwapchain, Uint size = 0);
    };
} // namespace Surge
