// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ParticleBindings.hpp"
#include "BindingUtils.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Client.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Surge/ECS/Components/Components.hpp"
#include "Surge/Graphics/Particles/ParticleEffect.hpp"
#include "Surge/ScriptEngine/Lua.hpp"

namespace Surge::ScriptBinding
{
    // The asset itself stays owned by the AssetManager
    static Ref<ParticleEffect> LoadEffect(const String& path)
    {
        AssetManager* am = Core::GetAssetManager();
        const AssetID id = am->GetIDFromPath(path);
        if(!id.IsValid() || am->GetMetadata(id).Type != AssetType::PARTICLE_EFFECT)
        {
            Log<Severity::Warn>("[ScriptEngine] '{}' is not a particle effect asset (paths are relative to the Assets folder)", path);
            return nullptr;
        }

        return am->Load<ParticleEffect>(id);
    }

    void BindParticles(void* luaState)
    {
        sol::state_view& lua = *static_cast<sol::state_view*>(luaState);

        lua.new_enum("ParticleCullingMode",
                     "ALWAYS_SIMULATE", ParticleCullingMode::ALWAYS_SIMULATE,
                     "PAUSE_OFFSCREEN", ParticleCullingMode::PAUSE_OFFSCREEN,
                     "PAUSE_AND_CATCH_UP", ParticleCullingMode::PAUSE_AND_CATCH_UP
        );

        lua.new_usertype<ParticleSystemComponent>("ParticleSystemComponent", sol::no_constructor,
                                                  "Active", BIND_PROP(ParticleSystemComponent, Active),
                                                  "AutoPlay", BIND_PROP(ParticleSystemComponent, AutoPlay),
                                                  "AutoDestroy", BIND_PROP(ParticleSystemComponent, AutoDestroy),
                                                  "SimulationSpeed", BIND_PROP(ParticleSystemComponent, SimulationSpeed),
                                                  "EmissionMultiplier", BIND_PROP(ParticleSystemComponent, EmissionMultiplier),
                                                  "Seed", BIND_PROP(ParticleSystemComponent, Seed),
                                                  "Culling", BIND_PROP(ParticleSystemComponent, Culling),
                                                  "IsPlaying", sol::property([](ParticleSystemComponent& c) { return c.Runtime.IsPlaying(); }),
                                                  "IsPaused", sol::property([](ParticleSystemComponent& c) { return c.Runtime.IsPaused(); }),
                                                  "IsAlive", sol::property([](ParticleSystemComponent& c) { return c.Runtime.IsAlive(); }),
                                                  "ParticleCount", sol::property([](ParticleSystemComponent& c) { return c.Runtime.GetParticleCount(); }),
                                                  "Play", [](ParticleSystemComponent& c) { c.Runtime.Play(); },
                                                  "Stop", [](ParticleSystemComponent& c, sol::optional<bool> clear) { c.Runtime.Stop(clear.value_or(false)); },
                                                  "Pause", [](ParticleSystemComponent& c) { c.Runtime.Pause(); },
                                                  "Restart", [](ParticleSystemComponent& c) { c.Runtime.Restart(); },
                                                  "Clear", [](ParticleSystemComponent& c) { c.Runtime.Clear(); },
                                                  "Emit", [](ParticleSystemComponent& c, int count) { if(count > 0) c.Runtime.Emit(static_cast<Uint>(count)); },
                                                  "SetEffect", [](ParticleSystemComponent& c, const String& effectPath) {
                                                      if(Ref<ParticleEffect> effect = LoadEffect(effectPath))
                                                          c.Effect = effect;
                                                  },
                                                  STRICT_READ(ParticleSystemComponent));

        // Particles: fire-and-forget effects in the running scene
        sol::table particles = lua.create_named_table("Particles");
        particles["Play"] = [](const String& effectPath, const glm::vec3& position, sol::optional<glm::vec3> rotation, sol::this_state s) -> sol::object {
            Client* client = Core::GetClient();
            Ref<Scene> scene = client ? client->GetRuntimeScene() : nullptr;
            Ref<ParticleEffect> effect = scene ? LoadEffect(effectPath) : nullptr;
            if(!effect)
                return sol::make_object(s, sol::lua_nil);

            Entity entity;
            scene->CreateEntity(entity, "Particle Effect");
            TransformComponent& transform = entity.GetComponent<TransformComponent>();
            transform.Position = position;
            transform.Rotation = rotation.value_or(glm::vec3(0.0f));
            transform.MarkDirty();

            ParticleSystemComponent& component = entity.AddComponent<ParticleSystemComponent>();
            component.Effect = effect;
            component.AutoPlay = true;
            component.AutoDestroy = true; // The entity goes away once the effect finished
            return sol::make_object(s, entity);
        };
    }
}
