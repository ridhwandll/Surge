// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class ShadowMapProcedure : public RenderProcedure
    {
    public:
        ShadowMapProcedure() = default;
        virtual ~ShadowMapProcedure() = default;

        virtual void Init(RendererData* rendererData) override;
        virtual void Update() override;
        virtual void Shutdown() override;

    public:
        struct InternalData
        {
            Ref<GraphicsPipeline> ShadowMapPipeline;
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
