// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderProcedure/RenderProcedure.hpp"

namespace Surge
{
    class GeometryProcedure : public RenderProcedure
    {
    public:
        GeometryProcedure() = default;
        virtual ~GeometryProcedure() = default;

        virtual void Init(RendererData* rendererData) override;
        virtual void Update() override;
        virtual void Shutdown() override;

    public:
        struct InternalData
        {
            Ref<GraphicsPipeline> GeometryPipeline;
            Ref<Framebuffer> OutputFrambuffer;
        };

    protected:
        virtual void* GetInternalDataBlock() override { return &mProcData; }

    private:
        InternalData mProcData;
        RendererData* mRendererData;

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge