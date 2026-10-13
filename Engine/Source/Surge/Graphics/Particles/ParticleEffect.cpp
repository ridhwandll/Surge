// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Particles/ParticleEffect.hpp"

namespace Surge
{
    void ParticleEmitterDesc::Bake()
    {
        Main.StartLifetime.Bake();
        Main.StartSpeed.Bake();
        Main.StartSize.Bake();
        Main.StartRotation.Bake();
        Main.StartColor.Bake();
        Emission.RateOverTime.Bake();
        Emission.RateOverDistance.Bake();
        Velocity.SpeedModifier.Bake();
        ColorOverLifetime.Gradient.Bake();
        SizeOverLifetime.Size.Bake();
        RotationOverLifetime.AngularVelocity.Bake();

        TextureSheet.Columns = std::max(TextureSheet.Columns, 1u);
        TextureSheet.Rows = std::max(TextureSheet.Rows, 1u);
        while(TextureSheet.Columns * TextureSheet.Rows > ParticleTextureSheetModule::MAX_FRAMES)
        {
            if(TextureSheet.Columns >= TextureSheet.Rows)
                TextureSheet.Columns--;
            else
                TextureSheet.Rows--;
        }

        Collision.PlaneCount = std::min(Collision.PlaneCount, ParticleCollisionModule::MAX_PLANES);
        for(ParticleCollisionPlane& plane : Collision.Planes)
        {
            const float length = glm::length(plane.Normal);
            plane.Normal = length > 1e-6f ? plane.Normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
        }

        Main.Duration = std::max(Main.Duration, 0.05f);
        Main.MaxParticles = std::max(Main.MaxParticles, 1u);
    }

    ParticleEffect::ParticleEffect()
    {
        // A new effect shows something right away: a soft upward cone
        ParticleEmitterDesc& emitter = Emitters.emplace_back();
        emitter.Name = "Emitter";
        emitter.Main.StartLifetime = ParticleFloat(2.0f);
        emitter.Main.StartSpeed = ParticleFloat(2.0f);
        emitter.Main.StartSize = ParticleFloat(0.5f);
        emitter.ColorOverLifetime.Enabled = true;
        emitter.ColorOverLifetime.Gradient.AlphaKeyCount = 3;
        emitter.ColorOverLifetime.Gradient.AlphaKeys[0] = { 0.0f, 0.0f };
        emitter.ColorOverLifetime.Gradient.AlphaKeys[1] = { 0.2f, 1.0f };
        emitter.ColorOverLifetime.Gradient.AlphaKeys[2] = { 1.0f, 0.0f };
        Bake();
    }

    void ParticleEffect::MarkChanged(bool layoutChanged)
    {
        Bake();
        mVersion++;
        if(layoutChanged)
            mLayoutVersion++;
    }

    void ParticleEffect::Bake()
    {
        if(Emitters.size() > MAX_EMITTERS)
            Emitters.resize(MAX_EMITTERS);

        for(ParticleEmitterDesc& emitter : Emitters)
        {
            emitter.Bake();
            emitter.IsSubEmitterTarget = false;
        }

        const int emitterCount = static_cast<int>(Emitters.size());
        for(int i = 0; i < emitterCount; i++)
        {
            const ParticleEmitterDesc& emitter = Emitters[i];
            if(!emitter.SubEmitters.Enabled)
                continue;

            for(const ParticleSubEmitter& entry : emitter.SubEmitters.Entries)
            {
                if(entry.EmitterIndex >= 0 && entry.EmitterIndex < emitterCount && entry.EmitterIndex != i)
                    Emitters[entry.EmitterIndex].IsSubEmitterTarget = true;
            }
        }
    }

} // namespace Surge
