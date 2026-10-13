// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "AssetCooker.hpp"
#include "Surge/Asset/Serializer/UILayout/UILayoutBinaryFormat.hpp"

namespace Surge
{
    class UILayoutCooker final : public AssetCooker
    {
    public:
        virtual CookResult Cook(const String& sourceAbsPath, AssetID id) const override;
        virtual AssetType GetAssetType() const override { return AssetType::UI_LAYOUT; }
        virtual Uint GetCookerVersion() const override { return UILayoutBinary::VERSION; }
    };
}
