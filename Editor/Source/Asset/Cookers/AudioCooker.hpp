// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "AssetCooker.hpp"
#include "Surge/Asset/Serializer/Audio/AudioBinaryFormat.hpp"

namespace Surge
{
    class AudioCooker final : public AssetCooker
    {
    public:
        virtual CookResult Cook(const String& sourceAbsPath, AssetID id) const override;
        virtual AssetType GetAssetType() const override { return AssetType::AUDIO; }
        virtual Uint GetCookerVersion() const override { return AudioBinary::VERSION; }
    };
}