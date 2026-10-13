// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Particles/ParticleCurve.hpp"

namespace Surge
{
    template<typename TKey, typename TValue, typename Getter>
    static TValue EvaluatePiecewise(const TKey* keys, Uint count, float t, const TValue& fallback, Getter&& get)
    {
        if(count == 0)
            return fallback;
        if(t <= keys[0].Time)
            return get(keys[0]);
        if(t >= keys[count - 1].Time)
            return get(keys[count - 1]);

        for(Uint i = 1; i < count; i++)
        {
            if(t <= keys[i].Time)
            {
                const float span = keys[i].Time - keys[i - 1].Time;
                const float f = span > 1e-6f ? (t - keys[i - 1].Time) / span : 1.0f;
                return glm::mix(get(keys[i - 1]), get(keys[i]), f);
            }
        }
        return get(keys[count - 1]);
    }

    template<typename TKey, size_t N>
    static void SortByTime(std::array<TKey, N>& keys, Uint count)
    {
        std::sort(keys.begin(), keys.begin() + count, [](const TKey& a, const TKey& b) { return a.Time < b.Time; });
    }

    template<typename TKey, size_t N>
    static bool InsertSorted(std::array<TKey, N>& keys, Uint& count, const TKey& key)
    {
        if(count >= N)
            return false;
        keys[count++] = key;
        SortByTime(keys, count);
        return true;
    }

    template<typename TKey, size_t N>
    static void RemoveAt(std::array<TKey, N>& keys, Uint& count, Uint index, Uint minCount)
    {
        if(index >= count || count <= minCount)
            return;
        for(Uint i = index; i + 1 < count; i++)
            keys[i] = keys[i + 1];
        count--;
    }

    //////////////////////////////////////////////////////////////////////////
    // ParticleCurve
    //////////////////////////////////////////////////////////////////////////

    void ParticleCurve::SetConstant(float value)
    {
        KeyCount = 2;
        Keys[0] = { 0.0f, value };
        Keys[1] = { 1.0f, value };
        Bake();
    }

    void ParticleCurve::SetLinear(float from, float to)
    {
        KeyCount = 2;
        Keys[0] = { 0.0f, from };
        Keys[1] = { 1.0f, to };
        Bake();
    }

    bool ParticleCurve::AddKey(float time, float value)
    {
        return InsertSorted(Keys, KeyCount, Key { glm::clamp(time, 0.0f, 1.0f), value });
    }

    void ParticleCurve::RemoveKey(Uint index)
    {
        RemoveAt(Keys, KeyCount, index, 1);
    }

    void ParticleCurve::SortKeys()
    {
        SortByTime(Keys, KeyCount);
    }

    float ParticleCurve::EvaluateKeys(float t) const
    {
        return EvaluatePiecewise(Keys.data(), KeyCount, t, 1.0f, [](const Key& k) { return k.Value; });
    }

    float ParticleCurve::GetMinValue() const
    {
        float result = KeyCount ? Keys[0].Value : 1.0f;
        for(Uint i = 1; i < KeyCount; i++)
            result = std::min(result, Keys[i].Value);
        return result;
    }

    float ParticleCurve::GetMaxValue() const
    {
        float result = KeyCount ? Keys[0].Value : 1.0f;
        for(Uint i = 1; i < KeyCount; i++)
            result = std::max(result, Keys[i].Value);
        return result;
    }

    void ParticleCurve::Bake()
    {
        for(Uint i = 0; i < LUT_SIZE; i++)
            mLUT[i] = EvaluateKeys(static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1));
    }

    //////////////////////////////////////////////////////////////////////////
    // ParticleGradient
    //////////////////////////////////////////////////////////////////////////

    ParticleGradient::ParticleGradient()
    {
        SetConstant(glm::vec4(1.0f));
    }

    void ParticleGradient::SetConstant(const glm::vec4& color)
    {
        ColorKeyCount = 2;
        ColorKeys[0] = { 0.0f, glm::vec3(color) };
        ColorKeys[1] = { 1.0f, glm::vec3(color) };
        AlphaKeyCount = 2;
        AlphaKeys[0] = { 0.0f, color.a };
        AlphaKeys[1] = { 1.0f, color.a };
        Bake();
    }

    bool ParticleGradient::AddColorKey(float time, const glm::vec3& color)
    {
        return InsertSorted(ColorKeys, ColorKeyCount, ColorKey { glm::clamp(time, 0.0f, 1.0f), color });
    }

    bool ParticleGradient::AddAlphaKey(float time, float alpha)
    {
        return InsertSorted(AlphaKeys, AlphaKeyCount, AlphaKey { glm::clamp(time, 0.0f, 1.0f), alpha });
    }

    void ParticleGradient::RemoveColorKey(Uint index)
    {
        RemoveAt(ColorKeys, ColorKeyCount, index, 1);
    }

    void ParticleGradient::RemoveAlphaKey(Uint index)
    {
        RemoveAt(AlphaKeys, AlphaKeyCount, index, 1);
    }

    void ParticleGradient::SortKeys()
    {
        SortByTime(ColorKeys, ColorKeyCount);
        SortByTime(AlphaKeys, AlphaKeyCount);
    }

    glm::vec4 ParticleGradient::EvaluateKeys(float t) const
    {
        const glm::vec3 color = EvaluatePiecewise(ColorKeys.data(), ColorKeyCount, t, glm::vec3(1.0f), [](const ColorKey& k) { return k.Color; });
        const float alpha = EvaluatePiecewise(AlphaKeys.data(), AlphaKeyCount, t, 1.0f, [](const AlphaKey& k) { return k.Alpha; });
        return glm::vec4(color, alpha);
    }

    void ParticleGradient::Bake()
    {
        for(Uint i = 0; i < LUT_SIZE; i++)
            mLUT[i] = EvaluateKeys(static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1));
    }

    //////////////////////////////////////////////////////////////////////////
    // ParticleFloat
    //////////////////////////////////////////////////////////////////////////

    float ParticleFloat::GetMaxValue() const
    {
        switch(Mode)
        {
            case ParticleValueMode::CONSTANT: return Max;
            case ParticleValueMode::RANDOM_BETWEEN_CONSTANTS: return std::max(Min, Max);
            case ParticleValueMode::CURVE: return CurveMax.GetMaxValue() * CurveMultiplier;
            case ParticleValueMode::RANDOM_BETWEEN_CURVES: return std::max(CurveMin.GetMaxValue(), CurveMax.GetMaxValue()) * CurveMultiplier;
        }
        return Max;
    }

    void ParticleFloat::Bake()
    {
        CurveMin.Bake();
        CurveMax.Bake();
    }

} // namespace Surge
