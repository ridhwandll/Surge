// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/DescriptorSet.hpp"

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

        void ResizeShadowMaps(Uint newSize)
        {
            for (Ref<Framebuffer>& framebuffer : mProcData.ShadowMapFramebuffers)
            {
                framebuffer->Resize(newSize, newSize);
                mProcData.ShadowMapResolution = newSize;
            }
        }

    public:
        struct InternalData
        {
            Vector<Ref<GraphicsPipeline>> ShadowMapPipelines;
            Vector<Ref<Framebuffer>> ShadowMapFramebuffers;

            Uint CascadeCount;
            float CascadeSplitLambda = 0.91f;
            bool VisualizeCascades;
            Vector<glm::mat4> LightViewProjections = {};
            Vector<float> CascadeSplitDepths = {};
            Uint ShadowMapResolution;

            Ref<UniformBuffer> ShadowUniformBuffer;
            Ref<DescriptorSet> ShadowDesciptorSet;
        };

    protected:
        virtual void* GetInternalDataBlock() override { return &mProcData; }

    private:
        void CalculateMatricesAndUpdateCBuffer(const glm::mat4& viewProjection, const glm::vec3& normalizedDirection);

    private:
        InternalData mProcData;
        RendererData* mRendererData;
        Vector<float> mCascadeSplits = {};

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge
