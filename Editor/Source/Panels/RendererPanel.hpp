// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Panels/RenderGraphTarget.hpp"
#include "Panels/RenderGraphVisualizer.hpp"

namespace Surge
{
    struct FrameBlackboard;

    // One window for everything rendering:
    //  - Settings: render settings, edited on the main blackboard (every view copies them each frame)
    //  - Graph:    the selected graph's groups/passes/barriers + a pass inspector (RenderGraphVisualizer)
    //  - Stats:    the selected graph's per-pass stats (RenderPass::OnImGuiRender) and the RHI metrics
    class RendererPanel : public IPanel
    {
    public:
        RendererPanel() = default;
        virtual ~RendererPanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent([[maybe_unused]] Event& e) override {}
        virtual void Render(bool* show) override;
        virtual void Shutdown() override {}

        static PanelCode GetStaticCode() { return PanelCode::Renderer; }

    private:
        void DrawSettings(FrameBlackboard& settings);
        void DrawStats(RenderGraph& graph);

    private:
        PanelCode mCode;
        RenderGraphTarget mTarget; // Shared by the Graph and Stats tabs
        RenderGraphVisualizer mVisualizer;

        bool mShowAdvanced = false;   // Knobs the quality presets drive
        float mRenderScale = 1.0f;    // Applied on release: every change rebuilds the scene targets of all graphs
        bool mEditingRenderScale = false;
    };

} // namespace Surge
