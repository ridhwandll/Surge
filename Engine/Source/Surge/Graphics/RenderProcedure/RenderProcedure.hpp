// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "SurgeReflect/SurgeReflect.hpp" // Needed

namespace Surge
{
    struct RendererData;
    class SURGE_API RenderProcedure
    {
    public:
        RenderProcedure() = default;
        virtual ~RenderProcedure() = default;

        virtual void Init(RendererData* rendererData) = 0;
        virtual void Update() = 0;
        virtual void Shutdown() = 0;
        virtual void Resize(Uint newWidth, Uint newHeight) = 0;

    protected:
        virtual void* GetInternalDataBlock() = 0;

    protected:
        RendererData* mRendererData;
        friend class SURGE_API RenderProcedureManager;
    };

} // namespace Surge