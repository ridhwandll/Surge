// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>

namespace Surge
{
    // Piecewise linear curve over [0, 1], up to MAX_KEYS keys. Bake() samples it into a lookup table: Sample() is one lerp per particle
    struct ParticleCurve
    {
        static constexpr Uint MAX_KEYS = 8;
        static constexpr Uint LUT_SIZE = 64;

        struct Key
        {
            float Time = 0.0f;
            float Value = 1.0f;
        };

        std::array<Key, MAX_KEYS> Keys = {};
        Uint KeyCount = 0;

        ParticleCurve() { SetConstant(1.0f); }

        void SetConstant(float value);
        void SetLinear(float from, float to);
        bool AddKey(float time, float value); // Keeps the keys sorted, false when full
        void RemoveKey(Uint index);
        void SortKeys();

        float EvaluateKeys(float t) const; // Exact, used by Bake() and the Editor
        float GetMinValue() const;
        float GetMaxValue() const;
        void Bake();

        FORCEINLINE float Sample(float t) const
        {
            const float x = glm::clamp(t, 0.0f, 1.0f) * static_cast<float>(LUT_SIZE - 1);
            const Uint i = std::min(static_cast<Uint>(x), LUT_SIZE - 2);
            return glm::mix(mLUT[i], mLUT[i + 1], x - static_cast<float>(i));
        }

    private:
        std::array<float, LUT_SIZE> mLUT = {};
    };

    // Color (RGB keys) + alpha keys over [0, 1], sRGB like every authored color, baked like ParticleCurve
    struct ParticleGradient
    {
        static constexpr Uint MAX_KEYS = 8;
        static constexpr Uint LUT_SIZE = 64;

        struct ColorKey
        {
            float Time = 0.0f;
            glm::vec3 Color = glm::vec3(1.0f);
        };

        struct AlphaKey
        {
            float Time = 0.0f;
            float Alpha = 1.0f;
        };

        std::array<ColorKey, MAX_KEYS> ColorKeys = {};
        Uint ColorKeyCount = 0;
        std::array<AlphaKey, MAX_KEYS> AlphaKeys = {};
        Uint AlphaKeyCount = 0;

        ParticleGradient();

        void SetConstant(const glm::vec4& color);
        bool AddColorKey(float time, const glm::vec3& color);
        bool AddAlphaKey(float time, float alpha);
        void RemoveColorKey(Uint index);
        void RemoveAlphaKey(Uint index);
        void SortKeys();

        glm::vec4 EvaluateKeys(float t) const;
        void Bake();

        FORCEINLINE glm::vec4 Sample(float t) const
        {
            const float x = glm::clamp(t, 0.0f, 1.0f) * static_cast<float>(LUT_SIZE - 1);
            const Uint i = std::min(static_cast<Uint>(x), LUT_SIZE - 2);
            return glm::mix(mLUT[i], mLUT[i + 1], x - static_cast<float>(i));
        }

    private:
        std::array<glm::vec4, LUT_SIZE> mLUT = {};
    };

    enum class ParticleValueMode : uint8_t
    {
        CONSTANT,
        RANDOM_BETWEEN_CONSTANTS,
        CURVE,                // Over the emitter duration (start values) or the particle lifetime
        RANDOM_BETWEEN_CURVES
    };

    // A float that is a constant, a random range or a curve. 'random' is a stable [0, 1) value per particle
    struct ParticleFloat
    {
        ParticleValueMode Mode = ParticleValueMode::CONSTANT;
        float Min = 0.0f;
        float Max = 0.0f; // The constant in CONSTANT mode
        float CurveMultiplier = 1.0f;
        ParticleCurve CurveMin;
        ParticleCurve CurveMax;

        ParticleFloat() = default;
        explicit ParticleFloat(float value) : Min(value), Max(value) {}

        FORCEINLINE float Evaluate(float t, float random) const
        {
            switch(Mode)
            {
                case ParticleValueMode::CONSTANT: return Max;
                case ParticleValueMode::RANDOM_BETWEEN_CONSTANTS: return glm::mix(Min, Max, random);
                case ParticleValueMode::CURVE: return CurveMax.Sample(t) * CurveMultiplier;
                case ParticleValueMode::RANDOM_BETWEEN_CURVES: return glm::mix(CurveMin.Sample(t), CurveMax.Sample(t), random) * CurveMultiplier;
            }
            return Max;
        }

        float GetMaxValue() const; // Upper bound over every t and random value (bounds, lifetimes)
        void Bake();
    };

    enum class ParticleColorMode : uint8_t
    {
        CONSTANT,
        RANDOM_BETWEEN_COLORS,
        GRADIENT,          // Over the emitter duration
        RANDOM_IN_GRADIENT
    };

    struct ParticleColor
    {
        ParticleColorMode Mode = ParticleColorMode::CONSTANT;
        glm::vec4 ColorA = glm::vec4(1.0f); // The constant in CONSTANT mode
        glm::vec4 ColorB = glm::vec4(1.0f);
        ParticleGradient Gradient;

        FORCEINLINE glm::vec4 Evaluate(float t, float random) const
        {
            switch(Mode)
            {
                case ParticleColorMode::CONSTANT: return ColorA;
                case ParticleColorMode::RANDOM_BETWEEN_COLORS: return glm::mix(ColorA, ColorB, random);
                case ParticleColorMode::GRADIENT: return Gradient.Sample(t);
                case ParticleColorMode::RANDOM_IN_GRADIENT: return Gradient.Sample(random);
            }
            return ColorA;
        }

        void Bake() { Gradient.Bake(); }
    };

} // namespace Surge
