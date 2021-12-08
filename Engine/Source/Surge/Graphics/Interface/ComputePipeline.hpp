// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "RenderCommandBuffer.hpp"

namespace Surge
{
    class ComputePipeline : public RefCounted
    {
    public:
        ComputePipeline() = default;
        virtual ~ComputePipeline() = default;

        virtual void Begin(const Ref<RenderCommandBuffer>& renderCmdBuffer) = 0;
        virtual void Dispatch(const Ref<RenderCommandBuffer>& renderCmdBuffer, Uint groupCountX, Uint groupCountY, Uint groupCountZ) = 0;
        virtual void End(const Ref<RenderCommandBuffer>& renderCmdBuffer) = 0;
        virtual const Ref<Shader>& GetShader() const = 0;

        static Ref<ComputePipeline> Create(Ref<Shader>& computeShader);
    };

} // namespace Surge
