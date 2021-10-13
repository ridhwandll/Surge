// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    enum class PanelCode
    {
        Viewport = 0,
        Performance
    };

    inline const char* PanelCodeToString(PanelCode code)
    {
        switch (code)
        {
            case Surge::PanelCode::Viewport: return "Viewport";
            case Surge::PanelCode::Performance: return "Performance";
        }
        return nullptr;
    }

} // namespace Surge
