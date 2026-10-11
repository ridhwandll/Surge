// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/String.hpp"
#include "Surge/Graphics/Renderer/RenderView.hpp"

namespace Surge
{
    class RenderGraph;

    // Which RenderGraph the render debug panels inspect. In the Editor the main graph only presents ImGui,
    // the scene is drawn by one graph per viewport (Renderer views)
    class RenderGraphTarget
    {
    public:
        // The graph to inspect this frame, never null. A closed viewport falls back to following the active Scene view
        RenderGraph* Resolve();
        void DrawSelector(float width);

        RenderViewID GetViewID() const { return mResolvedView; } // INVALID_RENDER_VIEW = main graph
        bool IsMainGraph() const { return mResolvedView == INVALID_RENDER_VIEW; }

    private:
        enum class Mode
        {
            ACTIVE_SCENE_VIEW,
            VIEW,
            MAIN
        };

        Mode mMode = Mode::ACTIVE_SCENE_VIEW;
        RenderViewID mView = INVALID_RENDER_VIEW; // Mode::VIEW
        RenderViewID mResolvedView = INVALID_RENDER_VIEW;
        Mode mLabelMode = Mode::MAIN;
        RenderViewID mLabelView = ~0u;            // Label is rebuilt only when the resolved target changes
        String mLabel;
    };

} // namespace Surge
