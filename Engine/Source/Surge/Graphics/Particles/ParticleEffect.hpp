// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Graphics/Particles/ParticleCurve.hpp"
#include "Surge/Graphics/Particles/ParticleRenderData.hpp"

namespace Surge
{
    enum class ParticleSimulationSpace : uint8_t
    {
        LOCAL, // Particles move with the entity
        WORLD  // Particles stay where they were emitted (trails, smoke behind a moving object)
    };

    enum class ParticleShapeType : uint8_t
    {
        POINT,
        SPHERE,
        HEMISPHERE,
        CONE,
        BOX,
        CIRCLE,
        EDGE
    };

    enum class ParticleSheetTiming : uint8_t
    {
        LIFETIME, // The sheet plays Cycles times over the particle lifetime
        FPS       // Fixed frame rate
    };

    enum class ParticleSubEmitterEvent : uint8_t
    {
        BIRTH,
        DEATH,
        COLLISION
    };

    // Start values are evaluated once per particle, curves in them run over the emitter duration
    struct ParticleMainModule
    {
        float Duration = 5.0f;
        bool Loop = true;
        bool Prewarm = false; // Looping emitters start as if they already ran a full cycle
        float StartDelay = 0.0f;

        ParticleFloat StartLifetime = ParticleFloat(5.0f);
        ParticleFloat StartSpeed = ParticleFloat(5.0f);
        ParticleFloat StartSize = ParticleFloat(1.0f);
        ParticleFloat StartRotation = ParticleFloat(0.0f); // Degrees
        ParticleColor StartColor;

        float GravityScale = 0.0f; // Times 9.81 m/s^2 down (world -Y)
        ParticleSimulationSpace SimulationSpace = ParticleSimulationSpace::LOCAL;
        Uint MaxParticles = 1000; // Pool size of the emitter, allocated once when the system starts
    };

    struct ParticleBurst
    {
        float Time = 0.0f; // Seconds into the emitter cycle
        Uint MinCount = 10;
        Uint MaxCount = 10;
        int Cycles = 1;        // 0 = repeat for the whole cycle
        float Interval = 0.1f; // Between two cycles
        float Probability = 1.0f;
    };

    struct ParticleEmissionModule
    {
        bool Enabled = true;
        ParticleFloat RateOverTime = ParticleFloat(10.0f);    // Particles per second
        ParticleFloat RateOverDistance = ParticleFloat(0.0f); // Particles per unit the emitter moved
        Vector<ParticleBurst> Bursts;
    };

    // Emits along +Y of the shape (cone, box, point, edge), radially for spheres and circles
    struct ParticleShapeModule
    {
        bool Enabled = true;
        ParticleShapeType Type = ParticleShapeType::CONE;
        float Radius = 1.0f;
        float RadiusThickness = 1.0f; // 0 = emit from the surface/edge only, 1 = the whole volume/area
        float Angle = 25.0f;          // Cone opening in degrees
        float Arc = 360.0f;           // Degrees of the circle/cone/sphere that emit
        glm::vec3 BoxSize = glm::vec3(1.0f);
        glm::vec3 Position = glm::vec3(0.0f); // Offset from the entity
        glm::vec3 Rotation = glm::vec3(0.0f); // Degrees
        float RandomizeDirection = 0.0f;      // 0 = shape direction, 1 = fully random
    };

    struct ParticleVelocityModule
    {
        bool Enabled = false;
        glm::vec3 Linear = glm::vec3(0.0f); // Added velocity, units per second
        ParticleSimulationSpace Space = ParticleSimulationSpace::LOCAL;
        ParticleCurve SpeedModifier;        // Multiplies the particle's own speed over its lifetime
    };

    struct ParticleLimitVelocityModule
    {
        bool Enabled = false;
        float SpeedLimit = 5.0f;
        float Dampen = 0.5f; // Fraction of the excess speed removed per second
        float Drag = 0.0f;   // Linear drag
    };

    struct ParticleForceModule
    {
        bool Enabled = false;
        glm::vec3 Force = glm::vec3(0.0f); // Acceleration, units per second^2
        ParticleSimulationSpace Space = ParticleSimulationSpace::WORLD;
    };

    struct ParticleNoiseModule
    {
        bool Enabled = false;
        float Strength = 1.0f;    // Units per second
        float Frequency = 0.5f;
        float ScrollSpeed = 0.5f; // Animates the noise field over time
    };

    struct ParticleColorOverLifetimeModule
    {
        bool Enabled = false;
        ParticleGradient Gradient; // Multiplies the start color
    };

    struct ParticleSizeOverLifetimeModule
    {
        bool Enabled = false;
        ParticleCurve Size; // Multiplies the start size
    };

    struct ParticleRotationOverLifetimeModule
    {
        bool Enabled = false;
        ParticleFloat AngularVelocity = ParticleFloat(45.0f); // Degrees per second, picked once per particle
    };

    struct ParticleTextureSheetModule
    {
        static constexpr Uint MAX_FRAMES = 256; // 8.8 fixed point frame in ParticleRecord

        bool Enabled = false;
        Uint Columns = 1;
        Uint Rows = 1;
        ParticleSheetTiming Timing = ParticleSheetTiming::LIFETIME;
        float Cycles = 1.0f; // LIFETIME timing
        float FPS = 30.0f;   // FPS timing
        bool RandomStartFrame = false;
        bool FrameBlending = false; // Cross fades to the next frame (two texture reads)
    };

    // World space plane: Dot(Normal, p) = Offset
    struct ParticleCollisionPlane
    {
        glm::vec3 Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        float Offset = 0.0f;
    };

    struct ParticleCollisionModule
    {
        static constexpr Uint MAX_PLANES = 4;

        bool Enabled = false;
        std::array<ParticleCollisionPlane, MAX_PLANES> Planes = {};
        Uint PlaneCount = 1;
        float Bounce = 0.5f;       // Fraction of the normal speed kept
        float Dampen = 0.1f;       // Fraction of the tangent speed lost per hit
        float LifetimeLoss = 0.0f; // Fraction of the lifetime lost per hit, 1 kills on contact
        float RadiusScale = 0.5f;  // Collision radius as a fraction of the particle size
    };

    struct ParticleSubEmitter
    {
        ParticleSubEmitterEvent Event = ParticleSubEmitterEvent::DEATH;
        int EmitterIndex = -1; // Another emitter of the same effect, usually with its emission disabled
        Uint Count = 5;
        float Probability = 1.0f;
        float InheritVelocity = 0.0f; // Fraction of the parent particle velocity added to the spawned ones
    };

    struct ParticleSubEmitterModule
    {
        bool Enabled = false;
        Vector<ParticleSubEmitter> Entries;
    };

    struct ParticleRendererModule
    {
        ParticleRenderMode Mode = ParticleRenderMode::BILLBOARD;
        Ref<Asset> Texture = nullptr;  // Texture2D, null = default soft particle
        float AdditiveAmount = 0.0f;   // 0 = alpha blended, 1 = additive
        float EmissiveIntensity = 1.0f;
        ParticleSortMode Sort = ParticleSortMode::NONE;
        int SortingOrder = 0;
        float MaxScreenSize = 0.5f;    // Fraction of the screen height, limits overdraw of particles close to the camera
        float StretchSpeedScale = 0.1f;
        float StretchLengthScale = 1.0f;
    };

    struct ParticleEmitterDesc
    {
        String Name = "Emitter";
        bool Enabled = true;

        ParticleMainModule Main;
        ParticleEmissionModule Emission;
        ParticleShapeModule Shape;
        ParticleVelocityModule Velocity;
        ParticleLimitVelocityModule LimitVelocity;
        ParticleForceModule Force;
        ParticleNoiseModule Noise;
        ParticleColorOverLifetimeModule ColorOverLifetime;
        ParticleSizeOverLifetimeModule SizeOverLifetime;
        ParticleRotationOverLifetimeModule RotationOverLifetime;
        ParticleTextureSheetModule TextureSheet;
        ParticleCollisionModule Collision;
        ParticleSubEmitterModule SubEmitters;
        ParticleRendererModule Renderer;

        bool IsSubEmitterTarget = false; // Set by ParticleEffect::Bake(): only emits when another emitter triggers it (no AutoPlay emission)

        void Bake(); // Lookup tables of every curve/gradient
        float GetMaxLifetime() const { return std::max(Main.StartLifetime.GetMaxValue(), 0.0f); }
    };

    // Particle effect asset (.spfx, human readable JSON, not cooked, loaded directly like .sui layouts)
    // Instances live in ParticleSystemComponent::Runtime, they rebuild when GetVersion() changes (Editor edits, reloads)
    class ParticleEffect final : public Asset
    {
    public:
        static constexpr Uint MAX_EMITTERS = 16;

        ParticleEffect();
        SURGE_ASSET_TYPE(AssetType::PARTICLE_EFFECT);
        static Ref<ParticleEffect> Create() { return Ref<ParticleEffect>::Create(); }

        // Call after editing: re-bakes the lookup tables and tells every running instance to rebuild. Parameter edits keep the live
        // particles, layoutChanged (emitters added, removed or reordered) restarts the instances
        void MarkChanged(bool layoutChanged = false);
        Uint GetVersion() const { return mVersion; }
        Uint GetLayoutVersion() const { return mLayoutVersion; }
        void Bake();

    public:
        Vector<ParticleEmitterDesc> Emitters;

    private:
        Uint mVersion = 1;
        Uint mLayoutVersion = 1;
    };

} // namespace Surge
