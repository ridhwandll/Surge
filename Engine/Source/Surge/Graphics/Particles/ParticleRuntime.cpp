// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Particles/ParticleRuntime.hpp"
#include "Surge/Graphics/Particles/ParticleEffect.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Graphics/HighLevel/Texture2D.hpp"
#include "Surge/ECS/Components/Components.hpp"
#include "Surge/Core/Profiler.hpp"
#include "SurgeMath/Frustum.hpp"

#include <glm/gtc/packing.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cfloat>

namespace Surge
{
    static constexpr float GRAVITY = 9.81f;
    static constexpr float MAX_STEP = 1.0f / 20.0f;     // Larger frame times are substepped (collisions stay stable)
    static constexpr float MAX_FRAME_TIME = 0.25f;      // A hitch never simulates more than this
    static constexpr float MAX_CATCH_UP_TIME = 1.0f;    // PAUSE_AND_CATCH_UP
    static constexpr float PREWARM_STEP = 1.0f / 30.0f;
    static constexpr float MAX_PREWARM_TIME = 10.0f;

    // Per particle random channels, derived from its seed so values stay stable over its lifetime
    enum RandomChannel : Uint
    {
        CHANNEL_LIFETIME,
        CHANNEL_SPEED,
        CHANNEL_SIZE,
        CHANNEL_ROTATION,
        CHANNEL_COLOR,
        CHANNEL_ANGULAR_VELOCITY,
        CHANNEL_START_FRAME
    };

    static FORCEINLINE Uint HashUint(Uint x)
    {
        // lowbias32 (Chris Wellons)
        x ^= x >> 16;
        x *= 0x7FEB352Du;
        x ^= x >> 15;
        x *= 0x846CA68Bu;
        x ^= x >> 16;
        return x;
    }

    static FORCEINLINE float ChannelRandom(Uint seed, Uint channel)
    {
        return static_cast<float>(HashUint(seed ^ (channel * 0x9E3779B9u)) >> 8) * (1.0f / 16777216.0f);
    }

    static FORCEINLINE glm::vec3 RandomUnitVector(float u, float v)
    {
        const float y = u * 2.0f - 1.0f;
        const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float theta = v * glm::two_pi<float>();
        return { r * std::cos(theta), y, r * std::sin(theta) };
    }

    // Cheap analytic turbulence, smooth in space and time
    static FORCEINLINE glm::vec3 Turbulence(const glm::vec3& p, float t)
    {
        return glm::vec3(glm::sin(p.y * 1.7f + t) * glm::cos(p.z * 1.3f - t * 0.8f) + glm::sin(p.z * 2.3f + t * 1.3f) * 0.5f,
                         glm::sin(p.z * 1.9f - t * 0.9f) * glm::cos(p.x * 1.1f + t * 1.2f) + glm::sin(p.x * 2.1f - t) * 0.5f,
                         glm::sin(p.x * 1.5f + t * 1.1f) * glm::cos(p.y * 1.7f - t * 0.7f) + glm::sin(p.y * 2.7f + t * 0.6f) * 0.5f) * (1.0f / 1.5f);
    }

    static FORCEINLINE bool LoopsForever(const ParticleEmitterDesc& desc)
    {
        return desc.Enabled && desc.Main.Loop && !desc.IsSubEmitterTarget;
    }

    // World space AABB of a simulation space box (Arvo's method)
    static void TransformBounds(const glm::vec3& min, const glm::vec3& max, const glm::mat4& transform, glm::vec3& outMin, glm::vec3& outMax)
    {
        const glm::vec3 center = glm::vec3(transform * glm::vec4((min + max) * 0.5f, 1.0f));
        const glm::vec3 extents = (max - min) * 0.5f;
        const glm::mat3 basis = glm::mat3(transform);
        const glm::vec3 worldExtents = glm::abs(basis[0]) * extents.x + glm::abs(basis[1]) * extents.y + glm::abs(basis[2]) * extents.z;
        outMin = center - worldExtents;
        outMax = center + worldExtents;
    }

    struct ParticleRuntime::StepContext
    {
        glm::mat4 Transform;
        glm::mat3 Basis;    // Rotation and scale of the entity
        glm::mat3 InvBasis;
        glm::vec3 EmitterPosition;
        float UniformScale;
        float EmissionScale;
        Uint Remaining;     // Particles this system may still spawn this frame
    };

    //////////////////////////////////////////////////////////////////////////
    // Lifetime
    //////////////////////////////////////////////////////////////////////////

    ParticleRuntime& ParticleRuntime::operator=(const ParticleRuntime& other)
    {
        if(this != &other)
            *this = ParticleRuntime(); // Copies start empty, same as the copy constructor
        return *this;
    }

    Uint ParticleRuntime::NextRandom()
    {
        // PCG32
        const uint64_t old = mRandomState;
        mRandomState = old * 6364136223846793005ULL + 1442695040888963407ULL;
        const Uint xorShifted = static_cast<Uint>(((old >> 18u) ^ old) >> 27u);
        const Uint rotation = static_cast<Uint>(old >> 59u);
        return (xorShifted >> rotation) | (xorShifted << ((0u - rotation) & 31u));
    }

    float ParticleRuntime::NextRandom01()
    {
        return static_cast<float>(NextRandom() >> 8) * (1.0f / 16777216.0f);
    }

    void ParticleRuntime::AllocatePool(EmitterState& state, Uint capacity)
    {
        // One allocation per type for the whole pool, nothing is allocated while the effect runs
        state.Capacity = capacity;
        state.Alive = 0;
        state.FloatData = capacity ? CreateScope<float[]>(static_cast<size_t>(capacity) * 11) : nullptr;
        state.UintData = capacity ? CreateScope<Uint[]>(static_cast<size_t>(capacity) * 2) : nullptr;

        float* floats = state.FloatData.get();
        state.PositionX = floats ? floats + capacity * 0 : nullptr;
        state.PositionY = floats ? floats + capacity * 1 : nullptr;
        state.PositionZ = floats ? floats + capacity * 2 : nullptr;
        state.VelocityX = floats ? floats + capacity * 3 : nullptr;
        state.VelocityY = floats ? floats + capacity * 4 : nullptr;
        state.VelocityZ = floats ? floats + capacity * 5 : nullptr;
        state.Age = floats ? floats + capacity * 6 : nullptr;
        state.InvLifetime = floats ? floats + capacity * 7 : nullptr;
        state.Size = floats ? floats + capacity * 8 : nullptr;
        state.Rotation = floats ? floats + capacity * 9 : nullptr;
        state.AngularVelocity = floats ? floats + capacity * 10 : nullptr;
        state.Color = state.UintData.get();
        state.Seed = state.UintData ? state.UintData.get() + capacity : nullptr;
    }

    void ParticleRuntime::Build(const ParticleEffect& effect, int seed)
    {
        SURGE_PROFILE_FUNC("ParticleRuntime::Build");

        // Same effect, same emitter list: a parameter edit (Editor) keeps the live particles and the playback running,
        // only emitters whose pool size changed start empty. Anything else starts over
        const bool sameLayout = mBuiltFrom == &effect && mBuiltLayoutVersion == effect.GetLayoutVersion() && mEmitters.size() == effect.Emitters.size();
        if(!sameLayout)
        {
            mEmitters.clear();
            mEmitters.resize(effect.Emitters.size());

            static Uint sSeedCounter = 0x2545F491u;
            const Uint baseSeed = seed != 0 ? static_cast<Uint>(seed) : HashUint(sSeedCounter++);
            mRandomState = (static_cast<uint64_t>(baseSeed) << 32) | HashUint(baseSeed + 1u);
            mHasWorldBounds = false;
            mCulled = false;
            mCulledTime = 0.0f;
        }

        for(size_t i = 0; i < effect.Emitters.size(); i++)
        {
            const ParticleEmitterDesc& desc = effect.Emitters[i];
            EmitterState& state = mEmitters[i];
            state.Bursts.resize(desc.Emission.Bursts.size());

            const Uint capacity = desc.Enabled ? desc.Main.MaxParticles : 0;
            if(state.Capacity != capacity)
                AllocatePool(state, capacity);

            // StepEmitter stops an emitter for good while it is disabled (or only spawns as a sub-emitter): when it comes back
            // during playback it starts its own cycle, the others keep running
            const bool active = desc.Enabled && !desc.IsSubEmitterTarget;
            if(sameLayout && mPlaying && active && !state.Active)
                ResetEmitterPlayback(state);
            state.Active = active;
        }

        mBuiltFrom = &effect;
        mBuiltVersion = effect.GetVersion();
        mBuiltLayoutVersion = effect.GetLayoutVersion();

        if(!sameLayout && mPlaying)
            ResetPlayback(); // Another effect or emitter list while playing (reloads, emitters added in the Editor): start over
    }

    void ParticleRuntime::ResetPlayback()
    {
        mTime = 0.0f;
        mPrewarmPending = true;
        for(EmitterState& state : mEmitters)
            ResetEmitterPlayback(state);
    }

    void ParticleRuntime::ResetEmitterPlayback(EmitterState& state)
    {
        state.Time = 0.0f;
        state.Cycle = -1; // Bursts restart on the next step
        state.EmitAccumulator = 0.0f;
        state.DistanceAccumulator = 0.0f;
        state.HasLastPosition = false;
        state.Emitting = true;
    }

    //////////////////////////////////////////////////////////////////////////
    // Playback
    //////////////////////////////////////////////////////////////////////////

    void ParticleRuntime::Play()
    {
        if(mPlaying && mPaused)
        {
            mPaused = false;
            return;
        }

        mPlaying = true;
        mPaused = false;
        mHasPlayed = true;
        mAutoPlayed = true; // An explicit Play() before the first update replaces AutoPlay
        ResetPlayback();
    }

    void ParticleRuntime::Stop(bool clear)
    {
        mPlaying = false;
        mPaused = false;
        for(EmitterState& state : mEmitters)
        {
            state.Emitting = false;
            if(clear)
                state.Alive = 0;
        }
    }

    void ParticleRuntime::Pause()
    {
        if(mPlaying || GetParticleCount() > 0)
            mPaused = true;
    }

    void ParticleRuntime::Restart()
    {
        Clear();
        mPlaying = false;
        Play();
    }

    void ParticleRuntime::Clear()
    {
        for(EmitterState& state : mEmitters)
            state.Alive = 0;
        mPendingEmit = 0;
    }

    void ParticleRuntime::Emit(Uint count)
    {
        mPendingEmit += count;
        mHasPlayed = true;
    }

    bool ParticleRuntime::IsAlive() const
    {
        if(mPendingEmit > 0)
            return true;

        for(const EmitterState& state : mEmitters)
        {
            if(state.Alive > 0 || (mPlaying && state.Emitting))
                return true;
        }
        return false;
    }

    bool ParticleRuntime::IsFinished() const
    {
        return mHasPlayed && !mPaused && !mEmitters.empty() && !IsAlive();
    }

    Uint ParticleRuntime::GetParticleCount() const
    {
        Uint count = 0;
        for(const EmitterState& state : mEmitters)
            count += state.Alive;
        return count;
    }

    //////////////////////////////////////////////////////////////////////////
    // Update
    //////////////////////////////////////////////////////////////////////////

    void ParticleRuntime::Update(const ParticleSystemComponent& component, const glm::mat4& transform, float deltaTime, const Frustum* frustum, ParticleBudget& budget)
    {
        const ParticleEffect* effect = nullptr;
        if(component.Effect && component.Effect->GetAssetType() == AssetType::PARTICLE_EFFECT)
            effect = static_cast<const ParticleEffect*>(component.Effect.Raw());

        if(!effect)
        {
            if(!mEmitters.empty())
            {
                mEmitters.clear();
                mBuiltFrom = nullptr;
            }
            mEffect = nullptr;
            return;
        }

        if(mBuiltFrom != effect || mBuiltVersion != effect->GetVersion() || mEmitters.size() != effect->Emitters.size())
            Build(*effect, component.Seed);

        mEffect = effect;
        mTransform = transform;

        if(component.AutoPlay && !mAutoPlayed)
            Play();

        StepContext ctx;
        ctx.Transform = transform;
        ctx.Basis = glm::mat3(transform);
        ctx.InvBasis = glm::inverse(ctx.Basis);
        ctx.EmitterPosition = glm::vec3(transform[3]);
        ctx.UniformScale = (glm::length(ctx.Basis[0]) + glm::length(ctx.Basis[1]) + glm::length(ctx.Basis[2])) * (1.0f / 3.0f);
        ctx.EmissionScale = budget.EmissionScale * std::max(component.EmissionMultiplier, 0.0f);

        const Uint systemAlive = GetParticleCount();
        const Uint remaining = budget.Remaining();
        ctx.Remaining = remaining > systemAlive ? remaining - systemAlive : 0;

        // Culling uses the bounds of the last step plus the current emitter position (a moving emitter comes back into view)
        if(frustum)
        {
            UpdateCullingBounds();
            const AABB bounds(mWorldBoundsMin, mWorldBoundsMax);
            mCulled = mHasWorldBounds && !frustum->IntersectsAABB(bounds, glm::mat4(1.0f));
        }
        else
            mCulled = false;

        float simulationTime = mPaused ? 0.0f : std::min(deltaTime * std::max(component.SimulationSpeed, 0.0f), MAX_FRAME_TIME);
        if(mCulled && component.Culling != ParticleCullingMode::ALWAYS_SIMULATE)
        {
            bool loops = false;
            for(const ParticleEmitterDesc& desc : effect->Emitters)
                loops |= LoopsForever(desc);

            // One-shots keep running off screen so they finish (and AutoDestroy can remove them)
            if(loops)
            {
                if(component.Culling == ParticleCullingMode::PAUSE_AND_CATCH_UP)
                    mCulledTime = std::min(mCulledTime + simulationTime, MAX_CATCH_UP_TIME);
                simulationTime = 0.0f;
            }
        }
        else if(!mCulled && mCulledTime > 0.0f)
        {
            simulationTime += mCulledTime;
            mCulledTime = 0.0f;
        }

        // Prewarm: looping emitters start as if a whole cycle already ran
        if(mPrewarmPending && mPlaying)
        {
            mPrewarmPending = false;
            for(Uint i = 0; i < static_cast<Uint>(mEmitters.size()); i++)
            {
                const ParticleEmitterDesc& desc = effect->Emitters[i];
                if(!desc.Main.Prewarm || !LoopsForever(desc))
                    continue;

                float prewarmTime = std::min(desc.Main.Duration, MAX_PREWARM_TIME);
                while(prewarmTime > 0.0f)
                {
                    const float step = std::min(prewarmTime, PREWARM_STEP);
                    StepEmitter(i, step, ctx);
                    prewarmTime -= step;
                }
            }
        }

        // Explicit Emit() calls
        if(mPendingEmit > 0)
        {
            for(Uint i = 0; i < static_cast<Uint>(mEmitters.size()); i++)
            {
                const ParticleEmitterDesc& desc = effect->Emitters[i];
                if(desc.Enabled && !desc.IsSubEmitterTarget)
                    EmitParticles(i, mPendingEmit, ctx, 0.0f, false);
            }
            mPendingEmit = 0;
        }

        while(simulationTime > 0.0f)
        {
            const float step = std::min(simulationTime, MAX_STEP);
            for(Uint i = 0; i < static_cast<Uint>(mEmitters.size()); i++)
                StepEmitter(i, step, ctx);

            mTime += step;
            simulationTime -= step;
        }

        if(!mPaused)
            UpdateCullingBounds();

        budget.Used += GetParticleCount();
    }

    void ParticleRuntime::UpdateCullingBounds()
    {
        mHasWorldBounds = false;
        if(!mEffect)
            return;

        const glm::vec3 origin = glm::vec3(mTransform[3]);
        for(size_t i = 0; i < mEmitters.size(); i++)
        {
            const ParticleEmitterDesc& desc = mEffect->Emitters[i];
            const EmitterState& state = mEmitters[i];
            if(!desc.Enabled)
                continue;

            glm::vec3 min, max;
            bool valid = false;
            if(state.Alive > 0)
            {
                // Particles are points in the pool, the largest one they can be grows the box
                const float sizeScale = desc.SizeOverLifetime.Enabled ? std::max(desc.SizeOverLifetime.Size.GetMaxValue(), 0.0f) : 1.0f;
                float halfSize = std::max(desc.Main.StartSize.GetMaxValue(), 0.0f) * sizeScale * 0.5f;
                if(desc.Renderer.Mode == ParticleRenderMode::STRETCHED)
                    halfSize *= std::max(desc.Renderer.StretchLengthScale, 1.0f) + desc.Main.StartSpeed.GetMaxValue() * desc.Renderer.StretchSpeedScale;

                const glm::vec3 inflate = glm::vec3(halfSize);
                if(desc.Main.SimulationSpace == ParticleSimulationSpace::LOCAL)
                    TransformBounds(state.BoundsMin - inflate, state.BoundsMax + inflate, mTransform, min, max);
                else
                {
                    min = state.BoundsMin - inflate;
                    max = state.BoundsMax + inflate;
                }
                valid = true;
            }

            // An emitting emitter is visible where it spawns
            if(mPlaying && state.Emitting && !desc.IsSubEmitterTarget)
            {
                const float reach = std::max({ desc.Shape.Radius, desc.Shape.BoxSize.x, desc.Shape.BoxSize.y, desc.Shape.BoxSize.z }) + std::max(desc.Main.StartSize.GetMaxValue(), 0.0f);
                const glm::vec3 shapeCenter = glm::vec3(mTransform * glm::vec4(desc.Shape.Position, 1.0f));
                min = valid ? glm::min(min, shapeCenter - reach) : shapeCenter - reach;
                max = valid ? glm::max(max, shapeCenter + reach) : shapeCenter + reach;
                valid = true;
            }

            if(!valid)
                continue;

            mWorldBoundsMin = mHasWorldBounds ? glm::min(mWorldBoundsMin, min) : min;
            mWorldBoundsMax = mHasWorldBounds ? glm::max(mWorldBoundsMax, max) : max;
            mHasWorldBounds = true;
        }

        if(!mHasWorldBounds)
        {
            mWorldBoundsMin = origin;
            mWorldBoundsMax = origin;
        }
    }

    void ParticleRuntime::StepEmitter(Uint index, float deltaTime, StepContext& ctx)
    {
        const ParticleEmitterDesc& desc = mEffect->Emitters[index];
        EmitterState& state = mEmitters[index];
        if(!desc.Enabled || state.Capacity == 0)
        {
            state.Emitting = false;
            return;
        }

        SimulateParticles(index, deltaTime, ctx);

        if(desc.IsSubEmitterTarget)
            state.Emitting = false; // Only spawns when another emitter triggers it
        if(!mPlaying || !state.Emitting)
            return;

        state.Time += deltaTime;
        const float emitterTime = state.Time - desc.Main.StartDelay;
        if(emitterTime < 0.0f)
            return;

        const float duration = desc.Main.Duration;
        if(!desc.Main.Loop && emitterTime - deltaTime >= duration)
        {
            state.Emitting = false;
            return;
        }

        const int cycle = desc.Main.Loop ? static_cast<int>(emitterTime / duration) : 0;
        const float cycleTime = desc.Main.Loop ? emitterTime - static_cast<float>(cycle) * duration : std::min(emitterTime, duration);
        const float normalizedTime = cycleTime / duration;
        if(cycle != state.Cycle)
        {
            state.Cycle = cycle;
            for(size_t b = 0; b < state.Bursts.size(); b++)
                state.Bursts[b] = { 0, desc.Emission.Bursts[b].Time };
        }

        Uint count = 0;
        if(desc.Emission.Enabled)
        {
            // Rate over time
            const float rate = std::max(desc.Emission.RateOverTime.Evaluate(normalizedTime, NextRandom01()), 0.0f);
            state.EmitAccumulator += rate * ctx.EmissionScale * deltaTime;

            // Rate over distance, the entity's movement since the last step
            const float distanceRate = desc.Emission.RateOverDistance.Evaluate(normalizedTime, NextRandom01());
            if(distanceRate > 0.0f && state.HasLastPosition)
                state.EmitAccumulator += glm::length(ctx.EmitterPosition - state.LastEmitterPosition) * distanceRate * ctx.EmissionScale;
            state.LastEmitterPosition = ctx.EmitterPosition;
            state.HasLastPosition = true;

            const float whole = std::floor(state.EmitAccumulator);
            state.EmitAccumulator -= whole;
            count += static_cast<Uint>(whole);

            // Bursts
            for(size_t b = 0; b < state.Bursts.size(); b++)
            {
                const ParticleBurst& burst = desc.Emission.Bursts[b];
                BurstState& burstState = state.Bursts[b];
                const float interval = std::max(burst.Interval, 0.01f);
                while((burst.Cycles <= 0 || burstState.CyclesDone < burst.Cycles) && cycleTime >= burstState.NextTime && burstState.NextTime <= duration)
                {
                    if(NextRandom01() < burst.Probability)
                    {
                        const Uint minCount = std::min(burst.MinCount, burst.MaxCount);
                        const Uint maxCount = std::max(burst.MinCount, burst.MaxCount);
                        const Uint burstCount = minCount + (maxCount > minCount ? NextRandom() % (maxCount - minCount + 1) : 0);
                        count += static_cast<Uint>(static_cast<float>(burstCount) * ctx.EmissionScale + 0.5f);
                    }
                    burstState.CyclesDone++;
                    burstState.NextTime += interval;
                }
            }
        }

        if(count > 0)
            EmitParticles(index, count, ctx, normalizedTime, false);

        if(!desc.Main.Loop && emitterTime >= duration)
            state.Emitting = false;
    }

    void ParticleRuntime::EmitParticles(Uint index, Uint count, StepContext& ctx, float emitterTime, bool fromSubEmitter)
    {
        const ParticleEmitterDesc& desc = mEffect->Emitters[index];
        EmitterState& state = mEmitters[index];

        count = std::min({ count, state.Capacity - state.Alive, ctx.Remaining });
        if(count == 0)
            return;
        ctx.Remaining -= count;

        const ParticleShapeModule& shape = desc.Shape;
        const glm::quat shapeRotation = glm::quat(glm::radians(shape.Rotation));
        const bool world = desc.Main.SimulationSpace == ParticleSimulationSpace::WORLD;
        const float arc = glm::radians(glm::clamp(shape.Arc, 0.0f, 360.0f));
        const float thickness = glm::clamp(shape.RadiusThickness, 0.0f, 1.0f);
        const float coneAngle = glm::radians(glm::clamp(shape.Angle, 0.0f, 90.0f));
        const bool rotationOverLifetime = desc.RotationOverLifetime.Enabled;

        for(Uint n = 0; n < count; n++)
        {
            const Uint seed = NextRandom();

            // Shape, emits along +Y
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
            if(shape.Enabled)
            {
                switch(shape.Type)
                {
                    case ParticleShapeType::POINT: break;
                    case ParticleShapeType::SPHERE:
                    case ParticleShapeType::HEMISPHERE:
                    {
                        const float u = shape.Type == ParticleShapeType::HEMISPHERE ? NextRandom01() * 0.5f + 0.5f : NextRandom01();
                        const float y = u * 2.0f - 1.0f;
                        const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
                        const float theta = NextRandom01() * arc;
                        direction = glm::vec3(r * std::cos(theta), y, r * std::sin(theta));

                        const float inner = 1.0f - thickness;
                        const float radius = shape.Radius * std::cbrt(glm::mix(inner * inner * inner, 1.0f, NextRandom01()));
                        position = direction * radius;
                        break;
                    }
                    case ParticleShapeType::CONE:
                    case ParticleShapeType::CIRCLE:
                    {
                        const float theta = NextRandom01() * arc;
                        const float inner = 1.0f - thickness;
                        const float rho = std::sqrt(glm::mix(inner * inner, 1.0f, NextRandom01())); // Uniform over the disc area
                        const glm::vec2 radial = { std::cos(theta), std::sin(theta) };
                        position = glm::vec3(radial.x, 0.0f, radial.y) * (rho * shape.Radius);
                        if(shape.Type == ParticleShapeType::CONE)
                        {
                            const float angle = coneAngle * rho;
                            direction = glm::vec3(std::sin(angle) * radial.x, std::cos(angle), std::sin(angle) * radial.y);
                        }
                        else
                            direction = glm::vec3(radial.x, 0.0f, radial.y);
                        break;
                    }
                    case ParticleShapeType::BOX:
                        position = (glm::vec3(NextRandom01(), NextRandom01(), NextRandom01()) - 0.5f) * shape.BoxSize;
                        break;
                    case ParticleShapeType::EDGE:
                        position = glm::vec3((NextRandom01() * 2.0f - 1.0f) * shape.Radius, 0.0f, 0.0f);
                        break;
                }

                if(shape.RandomizeDirection > 0.0f)
                {
                    const glm::vec3 randomDirection = RandomUnitVector(NextRandom01(), NextRandom01());
                    const glm::vec3 mixed = glm::mix(direction, randomDirection, glm::clamp(shape.RandomizeDirection, 0.0f, 1.0f));
                    const float length = glm::length(mixed);
                    direction = length > 1e-5f ? mixed / length : randomDirection;
                }

                position = shapeRotation * position + shape.Position;
                direction = shapeRotation * direction;
            }

            if(world)
            {
                position = glm::vec3(ctx.Transform * glm::vec4(position, 1.0f));
                const glm::vec3 worldDirection = ctx.Basis * direction;
                const float length = glm::length(worldDirection);
                direction = length > 1e-6f ? worldDirection / length : direction;
            }

            const float lifetime = std::max(desc.Main.StartLifetime.Evaluate(emitterTime, ChannelRandom(seed, CHANNEL_LIFETIME)), 0.01f);
            const float speed = desc.Main.StartSpeed.Evaluate(emitterTime, ChannelRandom(seed, CHANNEL_SPEED));
            const glm::vec3 velocity = direction * speed;
            const glm::vec4 color = glm::clamp(desc.Main.StartColor.Evaluate(emitterTime, ChannelRandom(seed, CHANNEL_COLOR)), 0.0f, 1.0f);

            const Uint i = state.Alive++;
            state.PositionX[i] = position.x;
            state.PositionY[i] = position.y;
            state.PositionZ[i] = position.z;
            state.VelocityX[i] = velocity.x;
            state.VelocityY[i] = velocity.y;
            state.VelocityZ[i] = velocity.z;
            state.Age[i] = 0.0f;
            state.InvLifetime[i] = 1.0f / lifetime;
            state.Size[i] = std::max(desc.Main.StartSize.Evaluate(emitterTime, ChannelRandom(seed, CHANNEL_SIZE)), 0.0f);
            state.Rotation[i] = glm::radians(desc.Main.StartRotation.Evaluate(emitterTime, ChannelRandom(seed, CHANNEL_ROTATION)));
            state.AngularVelocity[i] = rotationOverLifetime ? glm::radians(desc.RotationOverLifetime.AngularVelocity.Evaluate(0.0f, ChannelRandom(seed, CHANNEL_ANGULAR_VELOCITY))) : 0.0f;
            state.Color[i] = glm::packUnorm4x8(color);
            state.Seed[i] = seed;

            if(state.Alive == 1)
            {
                state.BoundsMin = position;
                state.BoundsMax = position;
            }
            else
            {
                state.BoundsMin = glm::min(state.BoundsMin, position);
                state.BoundsMax = glm::max(state.BoundsMax, position);
            }

            // Particles spawned by a sub-emitter never trigger birth events themselves (no emitter loops)
            if(!fromSubEmitter && desc.SubEmitters.Enabled)
                TriggerSubEmitters(index, static_cast<Uint>(ParticleSubEmitterEvent::BIRTH), position, velocity, ctx);
        }
    }

    void ParticleRuntime::KillParticle(EmitterState& state, Uint i)
    {
        const Uint last = --state.Alive;
        if(i == last)
            return;

        state.PositionX[i] = state.PositionX[last];
        state.PositionY[i] = state.PositionY[last];
        state.PositionZ[i] = state.PositionZ[last];
        state.VelocityX[i] = state.VelocityX[last];
        state.VelocityY[i] = state.VelocityY[last];
        state.VelocityZ[i] = state.VelocityZ[last];
        state.Age[i] = state.Age[last];
        state.InvLifetime[i] = state.InvLifetime[last];
        state.Size[i] = state.Size[last];
        state.Rotation[i] = state.Rotation[last];
        state.AngularVelocity[i] = state.AngularVelocity[last];
        state.Color[i] = state.Color[last];
        state.Seed[i] = state.Seed[last];
    }

    void ParticleRuntime::SimulateParticles(Uint index, float deltaTime, StepContext& ctx)
    {
        const ParticleEmitterDesc& desc = mEffect->Emitters[index];
        EmitterState& state = mEmitters[index];
        if(state.Alive == 0)
            return;

        const bool local = desc.Main.SimulationSpace == ParticleSimulationSpace::LOCAL;

        // Every acceleration and added velocity converted to the simulation space once, not per particle
        glm::vec3 acceleration = glm::vec3(0.0f, -GRAVITY * desc.Main.GravityScale, 0.0f);
        if(local)
            acceleration = ctx.InvBasis * acceleration;
        if(desc.Force.Enabled)
        {
            const bool forceLocal = desc.Force.Space == ParticleSimulationSpace::LOCAL;
            acceleration += forceLocal == local ? desc.Force.Force : (local ? ctx.InvBasis * desc.Force.Force : ctx.Basis * desc.Force.Force);
        }

        glm::vec3 addedVelocity = glm::vec3(0.0f);
        const bool velocityModule = desc.Velocity.Enabled;
        if(velocityModule)
        {
            const bool velocityLocal = desc.Velocity.Space == ParticleSimulationSpace::LOCAL;
            addedVelocity = velocityLocal == local ? desc.Velocity.Linear : (local ? ctx.InvBasis * desc.Velocity.Linear : ctx.Basis * desc.Velocity.Linear);
        }

        // Collision planes are authored in world space
        std::array<glm::vec4, ParticleCollisionModule::MAX_PLANES> planes;
        const Uint planeCount = desc.Collision.Enabled ? desc.Collision.PlaneCount : 0;
        for(Uint p = 0; p < planeCount; p++)
        {
            const ParticleCollisionPlane& plane = desc.Collision.Planes[p];
            glm::vec4 equation = glm::vec4(plane.Normal, -plane.Offset); // Dot(n, p) + w
            if(local)
            {
                // n . (M * p) + w = (M^T n) . p + w
                equation = glm::transpose(ctx.Transform) * equation;
                const float length = glm::length(glm::vec3(equation));
                equation = length > 1e-6f ? equation / length : glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            }
            planes[p] = equation;
        }

        const bool limit = desc.LimitVelocity.Enabled;
        const float speedLimit = std::max(desc.LimitVelocity.SpeedLimit, 0.0f);
        const float dampen = glm::clamp(desc.LimitVelocity.Dampen * deltaTime * 10.0f, 0.0f, 1.0f);
        const float drag = limit ? 1.0f / (1.0f + std::max(desc.LimitVelocity.Drag, 0.0f) * deltaTime) : 1.0f;
        const bool noise = desc.Noise.Enabled;
        const float noiseFrequency = desc.Noise.Frequency;
        const float noiseStrength = desc.Noise.Strength;
        const float noiseTime = mTime * desc.Noise.ScrollSpeed;
        const bool subEmitters = desc.SubEmitters.Enabled;
        const float collisionRadius = desc.Collision.RadiusScale * 0.5f;
        const float bounce = desc.Collision.Bounce;
        const float tangentKeep = 1.0f - glm::clamp(desc.Collision.Dampen, 0.0f, 1.0f);
        const float lifetimeLoss = desc.Collision.LifetimeLoss;

        glm::vec3 boundsMin = glm::vec3(FLT_MAX);
        glm::vec3 boundsMax = glm::vec3(-FLT_MAX);

        Uint i = 0;
        while(i < state.Alive)
        {
            const float age = state.Age[i] + deltaTime;
            const float t = age * state.InvLifetime[i];
            glm::vec3 position = { state.PositionX[i], state.PositionY[i], state.PositionZ[i] };
            glm::vec3 velocity = { state.VelocityX[i], state.VelocityY[i], state.VelocityZ[i] };

            if(t >= 1.0f)
            {
                if(subEmitters)
                    TriggerSubEmitters(index, static_cast<Uint>(ParticleSubEmitterEvent::DEATH), position, velocity, ctx);
                KillParticle(state, i);
                continue; // The last particle moved into slot i
            }

            velocity += acceleration * deltaTime;
            if(limit)
            {
                const float speed = glm::length(velocity);
                if(speed > speedLimit && speed > 1e-6f)
                    velocity *= glm::mix(1.0f, speedLimit / speed, dampen);
                velocity *= drag;
            }

            glm::vec3 moveVelocity = velocity;
            if(velocityModule)
                moveVelocity = velocity * desc.Velocity.SpeedModifier.Sample(t) + addedVelocity;
            if(noise)
                moveVelocity += Turbulence(position * noiseFrequency, noiseTime) * noiseStrength;

            position += moveVelocity * deltaTime;

            float newAge = age;
            for(Uint p = 0; p < planeCount; p++)
            {
                const glm::vec3 normal = glm::vec3(planes[p]);
                const float distance = glm::dot(normal, position) + planes[p].w - state.Size[i] * collisionRadius;
                const float normalSpeed = glm::dot(velocity, normal);
                if(distance >= 0.0f || normalSpeed >= 0.0f)
                    continue;

                position -= normal * distance;
                const glm::vec3 normalVelocity = normal * normalSpeed;
                velocity = (velocity - normalVelocity) * tangentKeep - normalVelocity * bounce;
                newAge += lifetimeLoss / state.InvLifetime[i];
                if(subEmitters)
                    TriggerSubEmitters(index, static_cast<Uint>(ParticleSubEmitterEvent::COLLISION), position, velocity, ctx);
            }

            state.PositionX[i] = position.x;
            state.PositionY[i] = position.y;
            state.PositionZ[i] = position.z;
            state.VelocityX[i] = velocity.x;
            state.VelocityY[i] = velocity.y;
            state.VelocityZ[i] = velocity.z;
            state.Age[i] = newAge;
            float rotation = state.Rotation[i] + state.AngularVelocity[i] * deltaTime;
            if(std::abs(rotation) > glm::two_pi<float>())
                rotation = std::fmod(rotation, glm::two_pi<float>()); // Packed as a half float, keep it small
            state.Rotation[i] = rotation;

            boundsMin = glm::min(boundsMin, position);
            boundsMax = glm::max(boundsMax, position);
            i++;
        }

        if(state.Alive > 0)
        {
            state.BoundsMin = boundsMin;
            state.BoundsMax = boundsMax;
        }
    }

    void ParticleRuntime::TriggerSubEmitters(Uint sourceIndex, Uint event, const glm::vec3& simPosition, const glm::vec3& simVelocity, StepContext& ctx)
    {
        const ParticleEmitterDesc& desc = mEffect->Emitters[sourceIndex];
        const bool local = desc.Main.SimulationSpace == ParticleSimulationSpace::LOCAL;

        for(const ParticleSubEmitter& entry : desc.SubEmitters.Entries)
        {
            if(static_cast<Uint>(entry.Event) != event || entry.EmitterIndex < 0 || entry.EmitterIndex >= static_cast<int>(mEmitters.size()) ||
               entry.EmitterIndex == static_cast<int>(sourceIndex) || entry.Count == 0)
                continue;
            if(entry.Probability < 1.0f && NextRandom01() >= entry.Probability)
                continue;

            const glm::vec3 worldPosition = local ? glm::vec3(ctx.Transform * glm::vec4(simPosition, 1.0f)) : simPosition;
            const glm::vec3 worldVelocity = (local ? ctx.Basis * simVelocity : simVelocity) * entry.InheritVelocity;
            SpawnAt(static_cast<Uint>(entry.EmitterIndex), worldPosition, worldVelocity, entry.Count, ctx);
        }
    }

    void ParticleRuntime::SpawnAt(Uint index, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, Uint count, StepContext& ctx)
    {
        const ParticleEmitterDesc& desc = mEffect->Emitters[index];
        EmitterState& state = mEmitters[index];
        if(!desc.Enabled || state.Capacity == 0)
            return;

        const Uint first = state.Alive;
        EmitParticles(index, count, ctx, 0.0f, true);

        // Move the new particles (shape around the target emitter) to the event position
        const bool local = desc.Main.SimulationSpace == ParticleSimulationSpace::LOCAL;
        const glm::vec3 anchor = glm::vec3(ctx.Transform * glm::vec4(desc.Shape.Position, 1.0f));
        const glm::vec3 offset = local ? ctx.InvBasis * (worldPosition - anchor) : worldPosition - anchor;
        const glm::vec3 addedVelocity = local ? ctx.InvBasis * worldVelocity : worldVelocity;
        for(Uint i = first; i < state.Alive; i++)
        {
            state.PositionX[i] += offset.x;
            state.PositionY[i] += offset.y;
            state.PositionZ[i] += offset.z;
            state.VelocityX[i] += addedVelocity.x;
            state.VelocityY[i] += addedVelocity.y;
            state.VelocityZ[i] += addedVelocity.z;

            const glm::vec3 position = { state.PositionX[i], state.PositionY[i], state.PositionZ[i] };
            state.BoundsMin = glm::min(state.BoundsMin, position);
            state.BoundsMax = glm::max(state.BoundsMax, position);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Submit
    //////////////////////////////////////////////////////////////////////////

    void ParticleRuntime::Submit(Renderer& renderer, RenderViewID view)
    {
        if(!mEffect || mCulled)
            return;

        SURGE_PROFILE_FUNC("ParticleRuntime::Submit");
        const glm::mat3 basis = glm::mat3(mTransform);
        const float uniformScale = (glm::length(basis[0]) + glm::length(basis[1]) + glm::length(basis[2])) * (1.0f / 3.0f);

        for(size_t e = 0; e < mEmitters.size(); e++)
        {
            const ParticleEmitterDesc& desc = mEffect->Emitters[e];
            const EmitterState& state = mEmitters[e];
            if(!desc.Enabled || state.Alive == 0)
                continue;

            const bool local = desc.Main.SimulationSpace == ParticleSimulationSpace::LOCAL;
            const ParticleTextureSheetModule& sheet = desc.TextureSheet;
            const ParticleRendererModule& rendererModule = desc.Renderer;

            ParticleEmitterCmd cmd;
            if(rendererModule.Texture && rendererModule.Texture->GetAssetType() == AssetType::TEXTURE2D)
                cmd.Texture = static_cast<const Texture2D*>(rendererModule.Texture.Raw())->GetRHIImage();

            cmd.Mode = rendererModule.Mode;
            cmd.Sort = rendererModule.Sort;
            cmd.SortingOrder = rendererModule.SortingOrder;
            cmd.FlipbookColumns = sheet.Enabled ? sheet.Columns : 1;
            cmd.FlipbookRows = sheet.Enabled ? sheet.Rows : 1;
            cmd.FrameBlending = sheet.Enabled && sheet.FrameBlending;
            cmd.AdditiveAmount = glm::clamp(rendererModule.AdditiveAmount, 0.0f, 1.0f);
            cmd.EmissiveIntensity = std::max(rendererModule.EmissiveIntensity, 0.0f);
            cmd.MaxScreenSize = rendererModule.MaxScreenSize > 0.0f ? rendererModule.MaxScreenSize : 1000.0f;
            cmd.StretchSpeedScale = rendererModule.StretchSpeedScale;
            cmd.StretchLengthScale = rendererModule.StretchLengthScale;
            cmd.BoundsCenter = (state.BoundsMin + state.BoundsMax) * 0.5f;
            if(local)
                cmd.BoundsCenter = glm::vec3(mTransform * glm::vec4(cmd.BoundsCenter, 1.0f));

            const ParticleSubmission submission = renderer.SubmitParticles(cmd, state.Alive, view);
            if(!submission.Records)
                continue;

            // Oldest/youngest first is camera independent, ordered here (distance sorting is per view, in ParticlePass)
            const Uint* order = nullptr;
            if(cmd.Sort == ParticleSortMode::OLDEST_FIRST || cmd.Sort == ParticleSortMode::YOUNGEST_FIRST)
            {
                mSortScratch.resize(state.Alive);
                for(Uint i = 0; i < state.Alive; i++)
                    mSortScratch[i] = i;

                const float* age = state.Age;
                if(cmd.Sort == ParticleSortMode::OLDEST_FIRST)
                    std::sort(mSortScratch.begin(), mSortScratch.end(), [age](Uint a, Uint b) { return age[a] > age[b]; });
                else
                    std::sort(mSortScratch.begin(), mSortScratch.end(), [age](Uint a, Uint b) { return age[a] < age[b]; });
                order = mSortScratch.data();
            }

            const bool sizeOverLifetime = desc.SizeOverLifetime.Enabled;
            const bool colorOverLifetime = desc.ColorOverLifetime.Enabled;
            const float sizeScale = local ? uniformScale : 1.0f;
            const float frameCount = static_cast<float>(cmd.FlipbookColumns * cmd.FlipbookRows);
            const bool animated = sheet.Enabled && frameCount > 1.0f;
            const Uint emitterBits = submission.EmitterIndex << 16;

            for(Uint n = 0; n < state.Alive; n++)
            {
                const Uint i = order ? order[n] : n;
                const float t = glm::clamp(state.Age[i] * state.InvLifetime[i], 0.0f, 1.0f);

                glm::vec3 position = { state.PositionX[i], state.PositionY[i], state.PositionZ[i] };
                glm::vec3 velocity = { state.VelocityX[i], state.VelocityY[i], state.VelocityZ[i] };
                if(local)
                {
                    position = glm::vec3(mTransform * glm::vec4(position, 1.0f));
                    velocity = basis * velocity;
                }

                float size = state.Size[i] * sizeScale;
                if(sizeOverLifetime)
                    size *= std::max(desc.SizeOverLifetime.Size.Sample(t), 0.0f);

                Uint color = state.Color[i];
                if(colorOverLifetime)
                    color = glm::packUnorm4x8(glm::clamp(glm::unpackUnorm4x8(color) * desc.ColorOverLifetime.Gradient.Sample(t), 0.0f, 1.0f));

                Uint frame = 0;
                if(animated)
                {
                    float f = sheet.Timing == ParticleSheetTiming::LIFETIME ? t * sheet.Cycles * frameCount : state.Age[i] * sheet.FPS;
                    if(sheet.RandomStartFrame)
                        f += std::floor(ChannelRandom(state.Seed[i], CHANNEL_START_FRAME) * frameCount);
                    f = std::fmod(std::max(f, 0.0f), frameCount);
                    frame = std::min(static_cast<Uint>(f * 256.0f), 0xFFFFu);
                }

                ParticleRecord& record = submission.Records[n];
                record.Position = position;
                record.Size = glm::packHalf2x16(glm::vec2(size));
                record.RotationFrame = static_cast<Uint>(glm::packHalf1x16(state.Rotation[i])) | (frame << 16);
                record.Color = color;
                record.VelocityXY = glm::packHalf2x16(glm::vec2(velocity.x, velocity.y));
                record.VelocityZEmitter = static_cast<Uint>(glm::packHalf1x16(velocity.z)) | emitterBits;
            }
        }
    }

} // namespace Surge
