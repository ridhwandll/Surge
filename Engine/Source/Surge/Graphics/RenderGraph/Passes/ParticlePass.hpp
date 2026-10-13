// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderGraph/RenderPass.hpp"
#include <unordered_map>

namespace Surge
{
    /*
    * ParticlePass:
    * Reads : Nothing (depth tested against the main pass, never written)
    * Writes: Blackboard.MainPassColorImage
    *
    * Draws FrameBlackboard::ParticleList inside the main render pass, after the opaque passes. One premultiplied alpha pipeline
    * covers alpha blended and additive particles. Prepare sorts the emitters back to front (SortingOrder first), distance sorts the
    * particles of DISTANCE emitters, uploads the records to a per-frame storage buffer and merges consecutive emitters with the
    * same texture into one draw. Quads are built in the vertex shader from the records (Particle.glsl, 6 vertices per particle)
    */
    class GraphicsRHI;
    class ParticlePass : public RenderPass
    {
    public:
        ParticlePass() { mName = "ParticlePass"; mGroup = PassGroup::MAIN_SCENE; }
        virtual ~ParticlePass() = default;

        void Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard) override;
        void Prepare(const FrameContext& ctx, FrameBlackboard& blackBoard) override;
        void Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard) override;
        bool HasWork(const FrameBlackboard& blackBoard) const override { return !blackBoard.ParticleEmitterList.empty(); }
        void Resize(Uint width, Uint height, FrameBlackboard& blackBoard) override;
        void OnImGuiRender(FrameBlackboard& blackBoard) override;
        void Shutdown(FrameBlackboard& blackBoard) override;

    private:
        // Mirrors EmitterData in Particle.glsl (std430)
        struct EmitterGPU
        {
            Uint Mode;
            Uint Columns;
            Uint Rows;
            Uint FrameBlending;
            float AdditiveAmount;
            float EmissiveIntensity;
            float MaxScreenSize;
            float StretchSpeedScale;
            float StretchLengthScale;
            float _pad0, _pad1, _pad2;
        };
        static_assert(sizeof(EmitterGPU) == 48, "EmitterGPU must mirror EmitterData in Particle.glsl");

        struct PushConstantData
        {
            float ProjectionScaleY; // Projection[1][1], converts world sizes to screen fractions (MaxScreenSize)
        };

        struct DrawCmd
        {
            DescriptorSetHandle TextureSet;
            Uint FirstParticle;
            Uint ParticleCount;
        };

        void EnsureCapacity(BufferHandle (&buffers)[RHISettings::FRAMES_IN_FLIGHT], Uint (&capacities)[RHISettings::FRAMES_IN_FLIGHT],
                            Uint frameIndex, Uint required, Uint stride, Uint binding, const char* debugName);
        DescriptorSetHandle GetTextureSet(ImageHandle image);
        void SortByDistance(const FrameBlackboard& blackBoard, const ParticleEmitterCmd& emitter);

    private:
        GraphicsRHI* mRHI = nullptr;
        PipelineHandle mPipeline;
        SamplerHandle mSampler;
        DescriptorSetHandle mFrameDescriptorSet;
        ImageHandle mDefaultTexture;

        BufferHandle mRecordBuffers[RHISettings::FRAMES_IN_FLIGHT];
        Uint mRecordCapacity[RHISettings::FRAMES_IN_FLIGHT] = {};
        BufferHandle mEmitterBuffers[RHISettings::FRAMES_IN_FLIGHT];
        Uint mEmitterCapacity[RHISettings::FRAMES_IN_FLIGHT] = {};

        // One descriptor set per texture, written once for every frame in flight and reused while the texture lives
        std::unordered_map<uint64_t, Pair<ImageHandle, DescriptorSetHandle>> mTextureSets;
        Uint mFramesSinceEviction = 0;

        // Scratch, reused every frame
        Vector<Uint> mEmitterOrder;
        Vector<EmitterGPU> mEmitterData;
        Vector<ParticleRecord> mSortedRecords;
        Vector<Pair<uint16_t, Uint>> mSortKeys;
        Vector<Pair<uint16_t, Uint>> mSortTemp;
        Vector<float> mSortDepths;
        Vector<DrawCmd> mDraws;
        float mProjectionScaleY = 1.0f;

        // Stats
        Uint mDrawnParticles = 0;
        Uint mDrawnEmitters = 0;
        Uint mSortedParticles = 0;
    };
}
