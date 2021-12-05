// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/DescriptorSet.hpp"
#define MAX_CASCADE_COUNT 4

namespace Surge
{
    enum class CascadeCount
    {
        TWO = 2,
        THREE = 3,
        FOUR = 4
    };
    FORCEINLINE Uint CascadeCountToUInt(CascadeCount c)
    {
        return static_cast<Uint>(c);
    }

    class ShadowMapProcedure : public RenderProcedure
    {
    public:
        ShadowMapProcedure();
        virtual ~ShadowMapProcedure() = default;

        virtual void Init(RendererData* rendererData) override;
        virtual void Update() override;
        virtual void Shutdown() override;

        // ShadowProc functions
        const CascadeCount& GetCascadeCount() const { return mTotalCascades; };
        void SetCascadeCount(CascadeCount count);
        FORCEINLINE void ResizeShadowMaps(Uint newSize)
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
            std::array<Ref<GraphicsPipeline>, MAX_CASCADE_COUNT> ShadowMapPipelines;
            std::array<Ref<Framebuffer>, MAX_CASCADE_COUNT> ShadowMapFramebuffers;
            std::array<glm::mat4, MAX_CASCADE_COUNT> LightViewProjections = {};
            std::array<float, MAX_CASCADE_COUNT> CascadeSplitDepths = {};

            float CascadeSplitLambda = 0.91f;
            bool VisualizeCascades = false;
            Uint ShadowMapResolution;

            Ref<UniformBuffer> ShadowUniformBuffer;
            Ref<DescriptorSet> ShadowDesciptorSet;
        };

    protected:
        virtual void* GetInternalDataBlock() override { return &mProcData; }

    private:
        void CalculateCascades(const glm::mat4& viewProjection, const glm::vec3& normalizedDirection);
        void UpdateShadowMapDescriptorSet();

    private:
        InternalData mProcData;
        RendererData* mRendererData;

        Surge::CascadeCount mTotalCascades;
        std::array<float, MAX_CASCADE_COUNT> mCascadeSplits = {};

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge
