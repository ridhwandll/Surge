// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "AssetCooker.hpp"
#include "Surge/Asset/Serializer/Scene/SceneBinaryFormat.hpp"

namespace Surge
{
    class SceneCooker final : public AssetCooker
    {
    public:
        virtual CookResult Cook(const String& sourceAbsPath, AssetID id) const override;
        virtual AssetType GetAssetType() const override { return AssetType::SCENE; }
        virtual Uint GetCookerVersion() const override { return SceneBinary::GetFormatVersion(); }
    };
}
