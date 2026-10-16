// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/RenderGraph/RenderPass.hpp"

namespace Surge
{
    /*
    * SwapchainPass:
    * if RHISettings::RENDER_TO_SWAPCHAIN == TRUE (Player)
    *    Reads : Nothing (PostProcess + UIOverlay already render into the swapchain group)
    * else (Editor)
    *    Reads : blackBoard.FinalImage (displayed by ImGui)
    * Writes: Nothing
    */

    class GraphicsRHI;
    class SwapchainPass : public RenderPass
    {
    public:
        SwapchainPass() { mName = "SwapchainPass"; mGroup = PassGroup::SWAPCHAIN; }
        virtual ~SwapchainPass() = default;

        void Setup(GraphicsRHI* rhi, FrameBlackboard& blackBoard) override;
        void Execute(const FrameContext& ctx, const FrameBlackboard& blackBoard) override;
        void Resize([[maybe_unused]] Uint width, [[maybe_unused]] Uint height, [[maybe_unused]] FrameBlackboard& blackBoard) override {};
        void RenderStatsViaImGui([[maybe_unused]] FrameBlackboard& blackBoard) override {};
        void Shutdown(FrameBlackboard& blackBoard) override;
    private:
        GraphicsRHI* mRHI = nullptr;
    };
}
