// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"

namespace Surge
{
    // Offscreen view of the Renderer (Editor Scene/Game viewports), see Renderer::CreateView()
    using RenderViewID = Uint;
    constexpr RenderViewID INVALID_RENDER_VIEW = 0;

    struct RenderViewDesc
    {
        String DebugName = "RenderView";
        bool EditorOverlay = false; // Also draws what was submitted after Renderer::BeginEditorOverlay() (editor debug geometry)
        bool ShowUI = false;        // In-game UI on top (UIOverlayPass)
        bool Isolated = false;      // Draws only what is submitted to this view (Renderer::SubmitParticles with its ID), never the frame's scene (previews)
    };

} // namespace Surge
