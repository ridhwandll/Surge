// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Graphics/Renderer/RenderView.hpp"
#include "Surge/Core/Scope.hpp"
#include <glm/glm.hpp>

namespace Surge
{
    class Renderer;
    class ParticleEffect;
    struct ParticleEmitterDesc;
    struct ParticleSubEmitter;
    struct ParticleSystemComponent;
    struct Frustum;

    enum class ParticleCullingMode : uint8_t
    {
        ALWAYS_SIMULATE,   // Simulates off screen too (only drawing is skipped)
        PAUSE_OFFSCREEN,   // Looping effects freeze off screen, one-shots keep running so they finish
        PAUSE_AND_CATCH_UP // Like PAUSE_OFFSCREEN, then simulates the missed time (up to a second) when visible again
    };

    // Live particle budget of a frame, shared by every particle system the scene updates (FrameBlackboard::Particles)
    struct ParticleBudget
    {
        Uint MaxParticles = 0;
        float EmissionScale = 1.0f;
        Uint Used = 0; // Running total, systems updated later get throttled first

        Uint Remaining() const { return Used < MaxParticles ? MaxParticles - Used : 0; }
    };

    // Live state of a ParticleSystemComponent: one particle pool (SoA) per emitter of the effect, the play state and the bounds.
    // Built from the effect on the first update and rebuilt when the effect changes. A copy starts empty (entering Play mode,
    // duplicating an entity), so particles are never shared between scenes
    class ParticleRuntime
    {
    public:
        ParticleRuntime() = default;
        ~ParticleRuntime() = default;
        ParticleRuntime(const ParticleRuntime&) {}
        ParticleRuntime& operator=(const ParticleRuntime& other);
        ParticleRuntime(ParticleRuntime&&) noexcept = default;
        ParticleRuntime& operator=(ParticleRuntime&&) noexcept = default;

        // Simulates 'deltaTime' aka scaled scene time in this case, 0 while paused. frustum = null disables culling (Editor views).
        // Components with AutoPlay start on their first update
        void Update(const ParticleSystemComponent& component, const glm::mat4& transform, float deltaTime, const Frustum* frustum, ParticleBudget& budget);

        // Packs the live particles of every visible emitter into the renderer's lists (or an isolated Editor view)
        void Submit(Renderer& renderer, RenderViewID view = INVALID_RENDER_VIEW);

        // Playback, valid before the first update too (applied once the effect is built)
        void Play();                   // Starts, or resumes when paused
        void Stop(bool clear = false); // Stops emitting, the live particles finish unless 'clear'
        void Pause();
        void Restart();
        void Clear();
        void Emit(Uint count);         // Into every emitter that is not a sub-emitter target

        bool IsPlaying() const { return mPlaying && !mPaused; }
        bool IsPaused() const { return mPaused; }
        bool IsAlive() const;    // Emitting or particles left
        bool IsFinished() const; // Played and done: AutoDestroy removes the entity
        bool IsCulled() const { return mCulled; }
        Uint GetParticleCount() const;
        float GetTime() const { return mTime; }

        // World space box used for frustum culling
        bool GetWorldBounds(glm::vec3& outMin, glm::vec3& outMax) const
        {
            outMin = mWorldBoundsMin;
            outMax = mWorldBoundsMax;
            return mHasWorldBounds;
        }

    private:
        struct BurstState
        {
            int CyclesDone = 0;
            float NextTime = 0.0f;
        };

        // Particle pool of one emitter, Capacity entries per array, live particles in [0, Alive)
        struct EmitterState
        {
            Scope<float[]> FloatData;
            Scope<Uint[]> UintData;
            float* PositionX = nullptr;
            float* PositionY = nullptr;
            float* PositionZ = nullptr;
            float* VelocityX = nullptr;
            float* VelocityY = nullptr;
            float* VelocityZ = nullptr;
            float* Age = nullptr;
            float* InvLifetime = nullptr;
            float* Size = nullptr;
            float* Rotation = nullptr;
            float* AngularVelocity = nullptr;
            Uint* Color = nullptr; // Start color, RGBA8
            Uint* Seed = nullptr;  // Stable per particle random values

            Uint Capacity = 0;
            Uint Alive = 0;

            Vector<BurstState> Bursts;
            float Time = 0.0f; // Since Play(), start delay included
            int Cycle = -1;
            float EmitAccumulator = 0.0f;
            float DistanceAccumulator = 0.0f;
            glm::vec3 LastEmitterPosition = glm::vec3(0.0f);
            bool HasLastPosition = false;
            bool Emitting = false;
            bool Active = false; // Enabled and not a sub-emitter target as of the last Build()

            glm::vec3 BoundsMin = glm::vec3(0.0f); // Simulation space, valid when Alive > 0
            glm::vec3 BoundsMax = glm::vec3(0.0f);
        };

        struct StepContext;

        void Build(const ParticleEffect& effect, int seed);
        static void AllocatePool(EmitterState& state, Uint capacity);
        void ResetPlayback();
        static void ResetEmitterPlayback(EmitterState& state);
        void StepEmitter(Uint index, float deltaTime, StepContext& ctx);
        void SimulateParticles(Uint index, float deltaTime, StepContext& ctx);
        void EmitParticles(Uint index, Uint count, StepContext& ctx, float emitterTime, bool fromSubEmitter);
        void TriggerSubEmitters(Uint sourceIndex, Uint event, const glm::vec3& simPosition, const glm::vec3& simVelocity, StepContext& ctx);
        void SpawnAt(Uint index, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, Uint count, StepContext& ctx);
        void KillParticle(EmitterState& state, Uint i);
        void UpdateCullingBounds();
        Uint NextRandom();
        float NextRandom01();

    private:
        Vector<EmitterState> mEmitters;
        const ParticleEffect* mEffect = nullptr; // Owned by the component, only valid during Update/Submit
        const void* mBuiltFrom = nullptr;
        Uint mBuiltVersion = 0;
        Uint mBuiltLayoutVersion = 0;

        uint64_t mRandomState = 0x853C49E6748FEA9BULL;
        glm::mat4 mTransform = glm::mat4(1.0f);
        glm::vec3 mWorldBoundsMin = glm::vec3(0.0f);
        glm::vec3 mWorldBoundsMax = glm::vec3(0.0f);
        bool mHasWorldBounds = false;

        float mTime = 0.0f;
        float mCulledTime = 0.0f;
        Uint mPendingEmit = 0;
        bool mPlaying = false;
        bool mPaused = false;
        bool mHasPlayed = false;
        bool mAutoPlayed = false;
        bool mPrewarmPending = false;
        bool mCulled = false;

        Vector<Uint> mSortScratch; // Submit(), oldest/youngest first ordering
    };

} // namespace Surge
