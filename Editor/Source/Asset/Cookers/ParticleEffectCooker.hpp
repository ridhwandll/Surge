// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "AssetCooker.hpp"
#include "Surge/Asset/Serializer/ParticleEffect/ParticleEffectBinaryFormat.hpp"

namespace Surge
{
    class ParticleEffectCooker final : public AssetCooker
    {
    public:
        virtual CookResult Cook(const String& sourceAbsPath, AssetID id) const override;
        virtual AssetType GetAssetType() const override { return AssetType::PARTICLE_EFFECT; }
        virtual Uint GetCookerVersion() const override { return ParticleEffectBinary::VERSION; }
    };
}
