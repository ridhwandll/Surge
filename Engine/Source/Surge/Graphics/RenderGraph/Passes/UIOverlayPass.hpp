// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderGraph/RenderPass.hpp"
#include "Renderer2DPass.hpp"

namespace Surge
{
    /*
    * UIOverlayPass:
    * Reads : Blackboard.FinalImage (Post-Processed SDR image)
    * Writes: Blackboard.UIOverlayFramebuffer
    *
    * Sprites and MSDF text go through one pipeline (UI.glsl) and are batched in Blackboard.UIDrawOrder,
    * so a panel drawn above a button also covers the button's label (hierarchy order is respected)
    */

    class GraphicsRHI;
    class UIOverlayPass : public RenderPass
    {
    public:
        // Player (RHISettings::RENDER_TO_SWAPCHAIN): drawn inside the swapchain render pass right after PostProcess
        UIOverlayPass() { mName = "UIOverlayPass"; mGroup = RHISettings::RENDER_TO_SWAPCHAIN ? PassGroup::SWAPCHAIN : PassGroup::UI_OVERLAY; }
        virtual ~UIOverlayPass() = default;

        void Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard) override;
        void Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard) override;
        void Resize(Uint width, Uint height, FrameBlackboard& blackBoard) override;
        void RenderStatsViaImGui(FrameBlackboard& blackBoard) override;
        void Shutdown(FrameBlackboard& blackBoard) override;

    public:
        static constexpr Uint MAX_UI_QUADS_TOTAL = 10000;
        static constexpr Uint MAX_UI_QUADS_PER_BATCH = 2500;
        static constexpr Uint MAX_UI_QUAD_BATCHES = 50;
        static constexpr Uint MAX_TEX_SLOTS_PER_BATCH = 16;

        // Packed into QuadVertex::TextureIndex, must match UI.glsl
        // bits 0-7: texture slot, bits 8-23: MSDF px range * 256 (text) or corner radius * 4 (sprites), bit 31: MSDF text glyph
        static constexpr Uint TEXT_GLYPH_FLAG = 0x80000000u;
        static constexpr Uint PX_RANGE_SHIFT = 8;
        static constexpr Uint CORNER_RADIUS_SHIFT = 8;
        static constexpr float CORNER_RADIUS_SCALE = 4.0f; // Quarter pixel precision, up to 16383 px

    private:
        void AppendSprite(const UISpriteCmd& sprite, const glm::vec4* clip);
        void AppendText(const TextSubmitCmd& text, const glm::vec4* clip);

        // positions/uvs in the order right-top, right-bottom, left-bottom, left-top (axis aligned), clip = optional (minX, minY, maxX, maxY)
        // Clipping happens here on the CPU: the quad shrinks and its UVs are interpolated, fully clipped quads are dropped
        // Returns false once the limits are hit
        bool PushQuad(glm::vec4 (&positions)[4], glm::vec2 (&uvs)[4], Uint packedColor, ImageHandle texture, Uint textureIndexFlags, const glm::vec4* clip);
        void FlushQuadBatch();

    private:
        GraphicsRHI* mRHI;
        FrameContext mCurrentFrameCtx;

        // Pipeline (Depth Disabled)
        PipelineHandle mUIPipeline;

        // Batching State
        Uint mTotalQuadVertexCount = 0;
        Uint mTotalQuadCount = 0;
        Uint mCurrentFrameVertexOffset = 0;
        bool mMaxQuadCountReached = false;

        std::array<Renderer2DPass::QuadDrawCmd, MAX_UI_QUAD_BATCHES> mQuadDrawCommands {};
        Uint mQuadBatchCount = 0;
        Vector<DescriptorSetHandle> mTexDescriptorSets;

        Renderer2DPass::QuadBatchData mCurrentQuadBatch;
        BufferHandle mQuadVB[RHISettings::FRAMES_IN_FLIGHT];
        BufferHandle mQuadIB;
        ImageHandle mWhiteImage;

        Vector<float> mLineLayoutCache;
    };
}
