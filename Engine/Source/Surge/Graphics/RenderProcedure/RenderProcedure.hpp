// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "SurgeReflect/SurgeReflect.hpp" // Needed

namespace Surge
{
    struct RendererData;
    class RenderProcedure
    {
    public:
        RenderProcedure() = default;
        virtual ~RenderProcedure() = default;

        virtual void Init(RendererData* rendererData) = 0;
        virtual void Update() = 0;
        virtual void Shutdown() = 0;

    protected:
        virtual void* GetInternalDataBlock() = 0;

    protected:
        RendererData* mRendererData;
        friend class RenderProcedureManager;
    };

} // namespace Surge