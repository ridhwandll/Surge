// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderProcedure/RenderProcedure.hpp"
#include "Surge/Graphics/Interface/ComputePipeline.hpp"
#include "Surge/Graphics/Interface/StorageBuffer.hpp"

namespace Surge
{
    class LightCullingProcedure : public RenderProcedure
    {
    public:
        LightCullingProcedure() = default;
        virtual ~LightCullingProcedure() = default;

        virtual void Init(RendererData* rendererData) override;
        virtual void Update() override;
        virtual void Shutdown() override;

    public:
        struct InternalData
        {
            Ref<ComputePipeline> LightCullingPipeline;
            Ref<DescriptorSet> LightListDescriptorSet;
            Ref<StorageBuffer> LightListStorageBuffer;
        };

    protected:
        virtual void* GetInternalDataBlock() override { return &mProcData; }

    private:
        InternalData mProcData;
        RendererData* mRendererData;

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge
