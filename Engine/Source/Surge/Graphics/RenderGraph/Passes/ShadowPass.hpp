// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderGraph/RenderPass.hpp"

namespace Surge
{
    /*
    * ShadowPass:
    * Reads : Nothing
    * Writes: Blackboard.ShadowPassFramebuffer
    */

    class GraphicsRHI;
    class ShadowPass : public RenderPass
    {
    public:
        ShadowPass() { mName = "ShadowPass"; mGroup = PassGroup::SHADOW; }
        virtual ~ShadowPass() = default;

        void Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard) override;
        void Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard) override;
        void Resize(Uint width, Uint height, FrameBlackboard& blackBoard) override;
        void OnImGuiRender(FrameBlackboard& blackBoard) override;
        void Shutdown(FrameBlackboard& blackBoard) override;
    private:
        void CalculateCascades(const FrameBlackboard& bb);
        void BuildCascadeDraws(const FrameContext& ctx, const FrameBlackboard& bb, Uint cascadeCount);
        void EnsureInstanceCapacity(Uint frameIndex, Uint instanceCount);

    private:
        struct ShadowDrawItem
        {
            const Mesh* Mesh_;
            Uint SubmeshIndex;
            Uint TransformIndex;
        };

        struct ShadowDraw
        {
            const Mesh* Mesh_;
            Uint SubmeshIndex;
            Uint FirstInstance;
            Uint InstanceCount;
        };

    private:
        GraphicsRHI* mRHI;

        // ShadowPass manages its own framebuffer
        FramebufferHandle mShadowPassFramebuffer[MAX_SHADOW_CASCADE_COUNT];

        PipelineHandle mShadowPipelines[MAX_SHADOW_CASCADE_COUNT];

        std::array<glm::mat4, MAX_SHADOW_CASCADE_COUNT> mLightViewProjections = {};
        std::array<float, MAX_SHADOW_CASCADE_COUNT> mCascadeSplits = {};
        std::array<float, MAX_SHADOW_CASCADE_COUNT> mCascadeEnds = {};
        float mShadowMapResolution;
        float mDepthBiasConstant = 1.25f;
        float mDepthBiasClamp = 0.0f;
        float mDepthBiasSlope = 1.75f;

        // Instancing: every cascade's visible casters, sorted by mesh, back to back in one per-frame buffer
        BufferHandle mInstanceBuffers[RHISettings::FRAMES_IN_FLIGHT];
        Uint mInstanceCapacity[RHISettings::FRAMES_IN_FLIGHT] = {};
        DescriptorSetHandle mInstanceSet;
        Vector<glm::mat4> mCasterTransforms;  // Scratch: world transforms of every caster submesh this frame
        Vector<ShadowDrawItem> mDrawItems;     // Scratch: visible items of one cascade
        Vector<glm::mat4> mInstanceData;       // Scratch: what goes into the instance buffer
        std::array<Vector<ShadowDraw>, MAX_SHADOW_CASCADE_COUNT> mCascadeDraws;
        std::array<Uint, MAX_SHADOW_CASCADE_COUNT> mCascadeVisible = {};
        Uint mCasterCount = 0;
    };
}
