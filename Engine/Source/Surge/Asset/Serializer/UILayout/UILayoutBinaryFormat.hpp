// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Asset/Serializer/AssetStamp.hpp"

namespace Surge
{
    class UILayout;
}

namespace Surge::UILayoutBinary
{
    // UILayoutBinary (.rUI_LAYOUT), the only layout format the runtime loads (the .sui JSON is the Editor's source)
    // AssetStamp
    // Header        Magic, Version
    // CanvasScaler  ReferenceResolution, MatchWidthOrHeight
    // WidgetDesc    pre-order tree: every field of the flat desc (defaults included), Options, ChildCount, then the children.
    //               Fonts/textures stay AssetIDs, resolved when the layout is instantiated
    // Little endian like every cooked format (x64, arm64)

    // Bump when a WidgetDesc/ImagePartDesc/CanvasScaler field is added/removed/reordered (also the UILayoutCooker version, so the Editor re-cooks every layout)
    inline constexpr Uint VERSION = 1;

    bool Write(const String& path, const AssetStamp& stamp, const UILayout& layout);
    bool Read(const String& path, AssetStamp& outStamp, UILayout& outLayout);
}
