// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class RenderCommandBuffer : public RefCounted
    {
    public:
        virtual ~RenderCommandBuffer() = default;

        virtual void BeginRecording() = 0;
        virtual void EndRecording() = 0;
        virtual void Submit() = 0;

        static Ref<RenderCommandBuffer> Create(Uint size = 0, bool createFromSwapchain = false, const String& debugName = "");
    };
}
