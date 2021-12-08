// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    enum class PanelCode
    {
        Viewport = 0,
        SceneHierarchy,
        Inspector,
        Performance,
        RenderProcedure
    };

    constexpr FORCEINLINE const char* PanelCodeToString(PanelCode code)
    {
        switch (code)
        {
            case PanelCode::Viewport: return "Viewport";
            case PanelCode::SceneHierarchy: return "Hierarchy";
            case PanelCode::Inspector: return "Inspector";
            case PanelCode::Performance: return "Performance";
            case PanelCode::RenderProcedure: return "RenderProcedure";
        }
        return nullptr;
    }

} // namespace Surge
