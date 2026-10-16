// Copyright (c) - SurgeTechnologies - All rights reserved
#include "MathBinding.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/ScriptEngine/Lua.hpp"
#include "SurgeMath/Math.hpp"

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/color_space.hpp>
#include <bit>
#include <cmath>
#include <format>
#include <limits>
#include <random>
#include <tuple>

namespace Surge::ScriptBinding
{
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TAU = 2.0 * PI;

    // xoshiro256** (same generator as Lua 5.4's math.random), own state so Math.SetSeed never touches math.random
    struct RandomGenerator
    {
        uint64_t State[4];

        void Seed(uint64_t seed)
        {
            for(uint64_t& s : State) // splitmix64
            {
                seed += 0x9E3779B97F4A7C15ull;
                uint64_t z = seed;
                z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
                z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
                s = z ^ (z >> 31);
            }
        }

        uint64_t Next()
        {
            const uint64_t result = Rotl(State[1] * 5, 7) * 9;
            const uint64_t t = State[1] << 17;
            State[2] ^= State[0];
            State[3] ^= State[1];
            State[1] ^= State[2];
            State[0] ^= State[3];
            State[2] ^= t;
            State[3] = Rotl(State[3], 45);
            return result;
        }

        double NextDouble() { return static_cast<double>(Next() >> 11) * 0x1.0p-53; } // [0, 1)
        float NextFloat() { return static_cast<float>(NextDouble()); }

        static uint64_t Rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    };
    static RandomGenerator sRandom;

    static void PushNumberOrInteger(lua_State* L, lua_Number value)
    {
        lua_Integer asInteger;
        if(lua_numbertointeger(value, &asInteger))
            lua_pushinteger(L, asInteger);
        else
            lua_pushnumber(L, value);
    }

    static double Repeat(double t, double length) { return length != 0.0 ? t - std::floor(t / length) * length : 0.0; }

    static double DeltaAngle(double from, double to)
    {
        const double delta = Repeat(to - from, TAU);
        return delta > PI ? delta - TAU : delta;
    }

    // (Rid) Scalars are plain lua_CFunctions on lua_Number (double), exactly like Lua's math library: same call cost
    // and precision, Floor/Ceil/Round/Trunc/Min/Max/Clamp keep integers integers. sol wrappers would add a functor
    // upvalue lookup + double->float->double round trips on every call.
    static const luaL_Reg sScalarFunctions[] = {
        // Trigonometry (radians)
        { "Sin",  [](lua_State* L) { lua_pushnumber(L, std::sin(luaL_checknumber(L, 1))); return 1; } },
        { "Cos",  [](lua_State* L) { lua_pushnumber(L, std::cos(luaL_checknumber(L, 1))); return 1; } },
        { "Tan",  [](lua_State* L) { lua_pushnumber(L, std::tan(luaL_checknumber(L, 1))); return 1; } },
        { "Asin", [](lua_State* L) { lua_pushnumber(L, std::asin(luaL_checknumber(L, 1))); return 1; } },
        { "Acos", [](lua_State* L) { lua_pushnumber(L, std::acos(luaL_checknumber(L, 1))); return 1; } },
        { "Atan", [](lua_State* L) { lua_pushnumber(L, std::atan2(luaL_checknumber(L, 1), luaL_optnumber(L, 2, 1.0))); return 1; } },
        { "Atan2", [](lua_State* L) { lua_pushnumber(L, std::atan2(luaL_checknumber(L, 1), luaL_checknumber(L, 2))); return 1; } },
        { "ToRadians", [](lua_State* L) { lua_pushnumber(L, luaL_checknumber(L, 1) * (PI / 180.0)); return 1; } },
        { "ToDegrees", [](lua_State* L) { lua_pushnumber(L, luaL_checknumber(L, 1) * (180.0 / PI)); return 1; } },

        // Exponents
        { "Sqrt", [](lua_State* L) { lua_pushnumber(L, std::sqrt(luaL_checknumber(L, 1))); return 1; } },
        { "Pow",  [](lua_State* L) { lua_pushnumber(L, std::pow(luaL_checknumber(L, 1), luaL_checknumber(L, 2))); return 1; } },
        { "Exp",  [](lua_State* L) { lua_pushnumber(L, std::exp(luaL_checknumber(L, 1))); return 1; } },
        { "Log",  [](lua_State* L)
            {
                const lua_Number x = luaL_checknumber(L, 1);
                if(lua_isnoneornil(L, 2))
                    lua_pushnumber(L, std::log(x));
                else
                {
                    const lua_Number base = luaL_checknumber(L, 2);
                    lua_pushnumber(L, base == 2.0 ? std::log2(x) : base == 10.0 ? std::log10(x) : std::log(x) / std::log(base));
                }
                return 1;
            } },

        // Rounding
        { "Abs", [](lua_State* L)
            {
                if(lua_isinteger(L, 1))
                {
                    const lua_Integer n = lua_tointeger(L, 1);
                    lua_pushinteger(L, n < 0 ? static_cast<lua_Integer>(0u - static_cast<lua_Unsigned>(n)) : n);
                }
                else
                    lua_pushnumber(L, std::fabs(luaL_checknumber(L, 1)));
                return 1;
            } },
        { "Sign", [](lua_State* L)
            {
                const lua_Number x = luaL_checknumber(L, 1);
                lua_pushinteger(L, (x > 0.0) - (x < 0.0));
                return 1;
            } },
        { "Floor", [](lua_State* L) { if(lua_isinteger(L, 1)) lua_settop(L, 1); else PushNumberOrInteger(L, std::floor(luaL_checknumber(L, 1))); return 1; } },
        { "Ceil",  [](lua_State* L) { if(lua_isinteger(L, 1)) lua_settop(L, 1); else PushNumberOrInteger(L, std::ceil(luaL_checknumber(L, 1))); return 1; } },
        { "Round", [](lua_State* L) { if(lua_isinteger(L, 1)) lua_settop(L, 1); else PushNumberOrInteger(L, std::round(luaL_checknumber(L, 1))); return 1; } },
        { "Trunc", [](lua_State* L) { if(lua_isinteger(L, 1)) lua_settop(L, 1); else PushNumberOrInteger(L, std::trunc(luaL_checknumber(L, 1))); return 1; } },
        { "Fract", [](lua_State* L) { const lua_Number x = luaL_checknumber(L, 1); lua_pushnumber(L, x - std::floor(x)); return 1; } },

        // Min / Max / Clamp
        { "Min", [](lua_State* L)
            {
                const int count = lua_gettop(L);
                luaL_argcheck(L, count >= 1, 1, "value expected");
                int best = 1;
                for(int i = 2; i <= count; i++)
                    if(lua_compare(L, i, best, LUA_OPLT))
                        best = i;
                lua_pushvalue(L, best);
                return 1;
            } },
        { "Max", [](lua_State* L)
            {
                const int count = lua_gettop(L);
                luaL_argcheck(L, count >= 1, 1, "value expected");
                int best = 1;
                for(int i = 2; i <= count; i++)
                    if(lua_compare(L, best, i, LUA_OPLT))
                        best = i;
                lua_pushvalue(L, best);
                return 1;
            } },
        { "Clamp", [](lua_State* L)
            {
                if(lua_isinteger(L, 1) && lua_isinteger(L, 2) && lua_isinteger(L, 3))
                {
                    const lua_Integer x = lua_tointeger(L, 1), lo = lua_tointeger(L, 2), hi = lua_tointeger(L, 3);
                    lua_pushinteger(L, x < lo ? lo : (x > hi ? hi : x));
                }
                else
                {
                    const lua_Number x = luaL_checknumber(L, 1), lo = luaL_checknumber(L, 2), hi = luaL_checknumber(L, 3);
                    lua_pushnumber(L, x < lo ? lo : (x > hi ? hi : x));
                }
                return 1;
            } },
        { "Clamp01", [](lua_State* L)
            {
                const lua_Number x = luaL_checknumber(L, 1);
                lua_pushnumber(L, x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x));
                return 1;
            } },

        // Interpolation
        { "Lerp", [](lua_State* L)
            {
                const lua_Number a = luaL_checknumber(L, 1), b = luaL_checknumber(L, 2), t = luaL_checknumber(L, 3);
                lua_pushnumber(L, a * (1.0 - t) + b * t);
                return 1;
            } },
        { "InverseLerp", [](lua_State* L)
            {
                const lua_Number a = luaL_checknumber(L, 1), b = luaL_checknumber(L, 2), value = luaL_checknumber(L, 3);
                lua_pushnumber(L, a != b ? (value - a) / (b - a) : 0.0);
                return 1;
            } },
        { "Remap", [](lua_State* L)
            {
                const lua_Number value = luaL_checknumber(L, 1);
                const lua_Number inMin = luaL_checknumber(L, 2), inMax = luaL_checknumber(L, 3);
                const lua_Number outMin = luaL_checknumber(L, 4), outMax = luaL_checknumber(L, 5);
                const lua_Number t = inMin != inMax ? (value - inMin) / (inMax - inMin) : 0.0;
                lua_pushnumber(L, outMin + (outMax - outMin) * t);
                return 1;
            } },
        { "SmoothStep", [](lua_State* L)
            {
                const lua_Number edge0 = luaL_checknumber(L, 1), edge1 = luaL_checknumber(L, 2), x = luaL_checknumber(L, 3);
                lua_Number t = edge0 != edge1 ? (x - edge0) / (edge1 - edge0) : (x < edge0 ? 0.0 : 1.0);
                t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
                lua_pushnumber(L, t * t * (3.0 - 2.0 * t));
                return 1;
            } },
        { "Step", [](lua_State* L) { lua_pushnumber(L, luaL_checknumber(L, 2) < luaL_checknumber(L, 1) ? 0.0 : 1.0); return 1; } },
        { "Repeat", [](lua_State* L) { lua_pushnumber(L, Repeat(luaL_checknumber(L, 1), luaL_checknumber(L, 2))); return 1; } },
        { "PingPong", [](lua_State* L)
            {
                const lua_Number length = luaL_checknumber(L, 2);
                const lua_Number t = Repeat(luaL_checknumber(L, 1), length * 2.0);
                lua_pushnumber(L, length - std::fabs(t - length));
                return 1;
            } },
        { "MoveTowards", [](lua_State* L)
            {
                const lua_Number current = luaL_checknumber(L, 1), target = luaL_checknumber(L, 2), maxDelta = luaL_checknumber(L, 3);
                const lua_Number delta = target - current;
                lua_pushnumber(L, std::fabs(delta) <= maxDelta ? target : current + (delta > 0.0 ? maxDelta : -maxDelta));
                return 1;
            } },
        { "DeltaAngle", [](lua_State* L) { lua_pushnumber(L, DeltaAngle(luaL_checknumber(L, 1), luaL_checknumber(L, 2))); return 1; } },
        { "LerpAngle", [](lua_State* L)
            {
                const lua_Number a = luaL_checknumber(L, 1), b = luaL_checknumber(L, 2), t = luaL_checknumber(L, 3);
                lua_pushnumber(L, a + DeltaAngle(a, b) * t);
                return 1;
            } },
        { "SmoothDamp", [](lua_State* L)
            {
                // Game Programming Gems 4, 1.10 (critically damped spring). Returns value, velocity
                const lua_Number current = luaL_checknumber(L, 1);
                lua_Number target = luaL_checknumber(L, 2);
                lua_Number velocity = luaL_checknumber(L, 3);
                const lua_Number smoothTime = std::fmax(0.0001, luaL_checknumber(L, 4));
                const lua_Number dt = luaL_checknumber(L, 5);
                const lua_Number maxSpeed = luaL_optnumber(L, 6, std::numeric_limits<double>::max());
                if(dt <= 0.0)
                {
                    lua_pushnumber(L, current);
                    lua_pushnumber(L, velocity);
                    return 2;
                }

                const lua_Number omega = 2.0 / smoothTime;
                const lua_Number x = omega * dt;
                const lua_Number decay = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x);
                const lua_Number originalTarget = target;
                const lua_Number maxChange = maxSpeed * smoothTime;
                const lua_Number change = std::fmin(std::fmax(current - target, -maxChange), maxChange);
                target = current - change;
                const lua_Number temp = (velocity + omega * change) * dt;
                velocity = (velocity - omega * temp) * decay;
                lua_Number output = target + (change + temp) * decay;
                if((originalTarget - current > 0.0) == (output > originalTarget)) // Overshot
                {
                    output = originalTarget;
                    velocity = 0.0;
                }
                lua_pushnumber(L, output);
                lua_pushnumber(L, velocity);
                return 2;
            } },
        { "Approximately", [](lua_State* L)
            {
                const lua_Number a = luaL_checknumber(L, 1), b = luaL_checknumber(L, 2);
                const lua_Number epsilon = luaL_optnumber(L, 3, 1e-6);
                lua_pushboolean(L, std::fabs(a - b) <= epsilon * std::fmax(1.0, std::fmax(std::fabs(a), std::fabs(b))));
                return 1;
            } },

        // Random (Math.SetSeed makes runs reproducible)
        { "Random", [](lua_State* L) { lua_pushnumber(L, sRandom.NextDouble()); return 1; } },
        { "RandomRange", [](lua_State* L)
            {
                const lua_Number lo = luaL_checknumber(L, 1), hi = luaL_checknumber(L, 2);
                lua_pushnumber(L, lo + (hi - lo) * sRandom.NextDouble());
                return 1;
            } },
        { "RandomInt", [](lua_State* L)
            {
                const lua_Integer lo = luaL_checkinteger(L, 1), hi = luaL_checkinteger(L, 2);
                luaL_argcheck(L, lo <= hi, 2, "interval is empty");
                const lua_Unsigned range = static_cast<lua_Unsigned>(hi) - static_cast<lua_Unsigned>(lo);
                const lua_Unsigned offset = range == ~lua_Unsigned(0) ? sRandom.Next() : sRandom.Next() % (range + 1); // Bias < range / 2^64
                lua_pushinteger(L, static_cast<lua_Integer>(static_cast<lua_Unsigned>(lo) + offset));
                return 1;
            } },
        { "SetSeed", [](lua_State* L) { sRandom.Seed(static_cast<uint64_t>(luaL_checkinteger(L, 1))); return 0; } },

        { nullptr, nullptr }
    };

    static constexpr double BACK_C1 = 1.70158;
    static constexpr double BACK_C3 = BACK_C1 + 1.0;

    static double BounceOut(double t)
    {
        constexpr double n = 7.5625, d = 2.75;
        if(t < 1.0 / d)
            return n * t * t;
        if(t < 2.0 / d)
        {
            t -= 1.5 / d;
            return n * t * t + 0.75;
        }
        if(t < 2.5 / d)
        {
            t -= 2.25 / d;
            return n * t * t + 0.9375;
        }
        t -= 2.625 / d;
        return n * t * t + 0.984375;
    }

    // t in [0, 1]
    static const luaL_Reg sEasingFunctions[] = {
        { "Linear",       [](lua_State* L) { lua_pushnumber(L, luaL_checknumber(L, 1)); return 1; } },
        { "QuadIn",       [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t * t); return 1; } },
        { "QuadOut",      [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t * (2.0 - t)); return 1; } },
        { "QuadInOut",    [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t); return 1; } },
        { "CubicIn",      [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t * t * t); return 1; } },
        { "CubicOut",     [](lua_State* L) { const double u = 1.0 - luaL_checknumber(L, 1); lua_pushnumber(L, 1.0 - u * u * u); return 1; } },
        { "CubicInOut",   [](lua_State* L) { const double t = luaL_checknumber(L, 1); const double u = 2.0 - 2.0 * t; lua_pushnumber(L, t < 0.5 ? 4.0 * t * t * t : 1.0 - u * u * u * 0.5); return 1; } },
        { "SineIn",       [](lua_State* L) { lua_pushnumber(L, 1.0 - std::cos(luaL_checknumber(L, 1) * PI * 0.5)); return 1; } },
        { "SineOut",      [](lua_State* L) { lua_pushnumber(L, std::sin(luaL_checknumber(L, 1) * PI * 0.5)); return 1; } },
        { "SineInOut",    [](lua_State* L) { lua_pushnumber(L, 0.5 - 0.5 * std::cos(luaL_checknumber(L, 1) * PI)); return 1; } },
        { "ExpoIn",       [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t <= 0.0 ? 0.0 : std::exp2(10.0 * t - 10.0)); return 1; } },
        { "ExpoOut",      [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, t >= 1.0 ? 1.0 : 1.0 - std::exp2(-10.0 * t)); return 1; } },
        { "BackIn",       [](lua_State* L) { const double t = luaL_checknumber(L, 1); lua_pushnumber(L, BACK_C3 * t * t * t - BACK_C1 * t * t); return 1; } },
        { "BackOut",      [](lua_State* L) { const double u = luaL_checknumber(L, 1) - 1.0; lua_pushnumber(L, 1.0 + BACK_C3 * u * u * u + BACK_C1 * u * u); return 1; } },
        { "ElasticOut",   [](lua_State* L)
            {
                const double t = luaL_checknumber(L, 1);
                lua_pushnumber(L, t <= 0.0 ? 0.0 : t >= 1.0 ? 1.0 : std::exp2(-10.0 * t) * std::sin((t * 10.0 - 0.75) * (TAU / 3.0)) + 1.0);
                return 1;
            } },
        { "BounceOut",    [](lua_State* L) { lua_pushnumber(L, BounceOut(luaL_checknumber(L, 1))); return 1; } },
        { "BounceIn",     [](lua_State* L) { lua_pushnumber(L, 1.0 - BounceOut(1.0 - luaL_checknumber(L, 1))); return 1; } },
        { nullptr, nullptr }
    };

    template <typename VecT>
    static VecT SafeNormalize(const VecT& v)
    {
        const float lengthSq = glm::dot(v, v);
        return lengthSq > 0.0f ? v * glm::inversesqrt(lengthSq) : VecT(0.0f);
    }

    template <typename VecT>
    static float AngleBetween(const VecT& a, const VecT& b)
    {
        const float denominator = glm::sqrt(glm::dot(a, a) * glm::dot(b, b));
        return denominator > 0.0f ? glm::acos(glm::clamp(glm::dot(a, b) / denominator, -1.0f, 1.0f)) : 0.0f;
    }

    // Everything Vec2/Vec3/Vec4 share. Methods never mutate except Set; operators allocate a new userdata (GC),
    // so hot loops should reuse one vector through Set or work on the numbers from Unpack
    template <typename VecT>
    static void BindVectorCommon(sol::usertype<VecT>& type)
    {
        type[sol::meta_function::addition] = [](const VecT& a, const VecT& b) { return a + b; };
        type[sol::meta_function::subtraction] = [](const VecT& a, const VecT& b) { return a - b; };
        type[sol::meta_function::multiplication] = sol::overload( // Most common first: overloads are tried in order
            [](const VecT& a, float s) { return a * s; },
            [](const VecT& a, const VecT& b) { return a * b; },
            [](float s, const VecT& a) { return a * s; });
        type[sol::meta_function::division] = sol::overload(
            [](const VecT& a, float s) { return a / s; },
            [](const VecT& a, const VecT& b) { return a / b; },
            [](float s, const VecT& a) { return s / a; });
        type[sol::meta_function::unary_minus] = [](const VecT& v) { return -v; };
        type[sol::meta_function::equal_to] = [](const VecT& a, const VecT& b) { return a == b; };

        type["Length"] = [](const VecT& v) { return glm::length(v); };
        type["LengthSquared"] = [](const VecT& v) { return glm::dot(v, v); };
        type["Normalize"] = [](const VecT& v) { return SafeNormalize(v); };
        type["Dot"] = [](const VecT& a, const VecT& b) { return glm::dot(a, b); };
        type["Distance"] = [](const VecT& a, const VecT& b) { return glm::distance(a, b); };
        type["DistanceSquared"] = [](const VecT& a, const VecT& b) { const VecT d = b - a; return glm::dot(d, d); };
        type["Angle"] = [](const VecT& a, const VecT& b) { return AngleBetween(a, b); };
        type["Lerp"] = [](const VecT& a, const VecT& b, float t) { return glm::mix(a, b, t); };
        type["MoveTowards"] = [](const VecT& current, const VecT& target, float maxDistance)
        {
            const VecT delta = target - current;
            const float distanceSq = glm::dot(delta, delta);
            if(distanceSq == 0.0f || (maxDistance >= 0.0f && distanceSq <= maxDistance * maxDistance))
                return target;
            return current + delta * (maxDistance * glm::inversesqrt(distanceSq));
        };
        type["ClampLength"] = [](const VecT& v, float maxLength)
        {
            const float lengthSq = glm::dot(v, v);
            return lengthSq > maxLength * maxLength ? v * (maxLength * glm::inversesqrt(lengthSq)) : v;
        };
        type["Project"] = [](const VecT& v, const VecT& onto)
        {
            const float ontoSq = glm::dot(onto, onto);
            return ontoSq > 0.0f ? onto * (glm::dot(v, onto) / ontoSq) : VecT(0.0f);
        };
        type["Reflect"] = [](const VecT& i, const VecT& n) { return glm::reflect(i, n); };
        type["Refract"] = [](const VecT& i, const VecT& n, float eta) { return glm::refract(i, n, eta); };
        type["Min"] = [](const VecT& a, const VecT& b) { return glm::min(a, b); };
        type["Max"] = [](const VecT& a, const VecT& b) { return glm::max(a, b); };
        type["Clamp"] = [](const VecT& v, const VecT& lo, const VecT& hi) { return glm::clamp(v, lo, hi); };
        type["Abs"] = [](const VecT& v) { return glm::abs(v); };
        type["Floor"] = [](const VecT& v) { return glm::floor(v); };
        type["Ceil"] = [](const VecT& v) { return glm::ceil(v); };
        type["Round"] = [](const VecT& v) { return glm::round(v); };
        type["Approximately"] = [](const VecT& a, const VecT& b, sol::optional<float> epsilon)
        {
            return glm::all(glm::lessThanEqual(glm::abs(a - b), VecT(epsilon.value_or(1e-6f))));
        };
        type["Clone"] = [](const VecT& v) { return v; };
    }

    static glm::quat LookRotation(const glm::vec3& forward, const glm::vec3& up)
    {
        const float lengthSq = glm::dot(forward, forward);
        if(lengthSq < glm::epsilon<float>())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        const glm::vec3 f = forward * glm::inversesqrt(lengthSq);
        glm::vec3 u = up;
        if(glm::dot(glm::cross(f, u), glm::cross(f, u)) < 1e-10f) // Forward parallel to up: lookAt would produce NaNs
            u = glm::abs(f.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

        return glm::conjugate(glm::quat_cast(glm::lookAt(glm::vec3(0.0f), f, u)));
    }

    static glm::vec4 HexToColor(const String& hex)
    {
        // "#RRGGBB", "RRGGBB", "#RRGGBBAA" or "RRGGBBAA". Invalid input is white
        const size_t start = !hex.empty() && hex[0] == '#' ? 1 : 0;
        const size_t digits = hex.size() - start;
        if(digits != 6 && digits != 8)
            return glm::vec4(1.0f);

        uint32_t value = 0;
        for(size_t i = start; i < hex.size(); i++)
        {
            const char c = hex[i];
            const uint32_t nibble = c >= '0' && c <= '9' ? static_cast<uint32_t>(c - '0') :
                                    c >= 'a' && c <= 'f' ? static_cast<uint32_t>(c - 'a' + 10) :
                                    c >= 'A' && c <= 'F' ? static_cast<uint32_t>(c - 'A' + 10) : 16u;
            if(nibble > 15)
                return glm::vec4(1.0f);
            value = (value << 4) | nibble;
        }

        if(digits == 6)
            value = (value << 8) | 0xFF;
        return glm::vec4((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF) / 255.0f;
    }

    void BindMath(void* luaState)
    {
        sol::state_view* lua = static_cast<sol::state_view*>(luaState);
        lua_State* L = lua->lua_state();

        sRandom.Seed((static_cast<uint64_t>(std::random_device{}()) << 32) ^ std::random_device{}());

        // Global Math namespace in Lua
        auto math = lua->create_table("Math");

        math["Pi"] = PI;
        math["Tau"] = TAU;
        math["Epsilon"] = glm::epsilon<float>();
        math["Infinity"] = std::bit_cast<double>(0x7FF0000000000000ull); // math.huge (HUGE_VAL warns under /fp:fast)
        math["Deg2Rad"] = PI / 180.0;
        math["Rad2Deg"] = 180.0 / PI;
        math["MaxInteger"] = LUA_MAXINTEGER;
        math["MinInteger"] = LUA_MININTEGER;

        math.push();
        luaL_setfuncs(L, sScalarFunctions, 0);
        lua_pop(L, 1);

        sol::table easing = lua->create_table();
        math["Easing"] = easing;
        easing.push();
        luaL_setfuncs(L, sEasingFunctions, 0);
        lua_pop(L, 1);

        // Vector 2
        sol::usertype<glm::vec2> vec2 = math.new_usertype<glm::vec2>("Vec2",
            sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            sol::meta_function::to_string, [](const glm::vec2& v) { return std::format("Vec2({:.3f}, {:.3f})", v.x, v.y); },

            "Set",           [](glm::vec2& v, float x, float y) { v = glm::vec2(x, y); },
            "Unpack",        [](const glm::vec2& v) { return std::make_tuple(v.x, v.y); },
            "Cross",         [](const glm::vec2& a, const glm::vec2& b) { return a.x * b.y - a.y * b.x; },
            "Perpendicular", [](const glm::vec2& v) { return glm::vec2(-v.y, v.x); },
            "SignedAngle",   [](const glm::vec2& a, const glm::vec2& b) { return glm::atan(a.x * b.y - a.y * b.x, glm::dot(a, b)); },
            "Rotate",        [](const glm::vec2& v, float angleRad)
                             {
                                 const float c = glm::cos(angleRad), s = glm::sin(angleRad);
                                 return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
                             }
        );
        BindVectorCommon(vec2);

        // Vector 3
        sol::usertype<glm::vec3> vec3 = math.new_usertype<glm::vec3>("Vec3",
            sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float), glm::vec3(const glm::vec2&, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            sol::meta_function::to_string, [](const glm::vec3& v) { return std::format("Vec3({:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z); },

            "Set",            [](glm::vec3& v, float x, float y, float z) { v = glm::vec3(x, y, z); },
            "Unpack",         [](const glm::vec3& v) { return std::make_tuple(v.x, v.y, v.z); },
            "XY",             [](const glm::vec3& v) { return glm::vec2(v); },
            "Cross",          [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
            "SignedAngle",    [](const glm::vec3& a, const glm::vec3& b, const glm::vec3& axis)
                              {
                                  const float angle = AngleBetween(a, b);
                                  return glm::dot(axis, glm::cross(a, b)) < 0.0f ? -angle : angle;
                              },
            "ProjectOnPlane", [](const glm::vec3& v, const glm::vec3& normal)
                              {
                                  const float normalSq = glm::dot(normal, normal);
                                  return normalSq > 0.0f ? v - normal * (glm::dot(v, normal) / normalSq) : v;
                              }
        );
        BindVectorCommon(vec3);

        // Vector 4 (also colors: x,y,z,w = r,g,b,a)
        sol::usertype<glm::vec4> vec4 = math.new_usertype<glm::vec4>("Vec4",
            sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float), glm::vec4(const glm::vec3&, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w,
            sol::meta_function::to_string, [](const glm::vec4& v) { return std::format("Vec4({:.3f}, {:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z, v.w); },

            "Set",    [](glm::vec4& v, float x, float y, float z, float w) { v = glm::vec4(x, y, z, w); },
            "Unpack", [](const glm::vec4& v) { return std::make_tuple(v.x, v.y, v.z, v.w); },
            "XY",     [](const glm::vec4& v) { return glm::vec2(v); },
            "XYZ",    [](const glm::vec4& v) { return glm::vec3(v); }
        );
        BindVectorCommon(vec4);

        // Quaternions (Rotations)
        math.new_usertype<glm::quat>("Quat",
            sol::no_constructor,
            // glm's own 4 float constructor is (w, x, y, z): Lua gets (x, y, z, w) like the fields
            "new", sol::overload(
                []() { return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); },
                [](float x, float y, float z, float w) { return glm::quat(w, x, y, z); },
                [](const glm::vec3& eulerRad) { return glm::quat(eulerRad); }),
            "x", &glm::quat::x,
            "y", &glm::quat::y,
            "z", &glm::quat::z,
            "w", &glm::quat::w,

            // Operators
            sol::meta_function::multiplication, sol::overload(
                [](const glm::quat& q, const glm::vec3& v) { return q * v; }, // Rotate vector
                [](const glm::quat& a, const glm::quat& b) { return a * b; }  // Combine rotations
            ),
            sol::meta_function::equal_to,  [](const glm::quat& a, const glm::quat& b) { return a == b; },
            sol::meta_function::to_string, [](const glm::quat& q) { return std::format("Quat({:.3f}, {:.3f}, {:.3f}, {:.3f})", q.x, q.y, q.z, q.w); },

            // Constructors
            "Identity",       []() { return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); },
            "FromEuler",      [](const glm::vec3& eulerRad) { return glm::quat(eulerRad); },
            "AngleAxis",      [](float angleRad, const glm::vec3& axis) { return glm::angleAxis(angleRad, SafeNormalize(axis)); },
            "FromToRotation", [](const glm::vec3& from, const glm::vec3& to) { return glm::rotation(SafeNormalize(from), SafeNormalize(to)); },
            "LookRotation",   [](const glm::vec3& forward, sol::optional<glm::vec3> up) { return LookRotation(forward, up.value_or(glm::vec3(0.0f, 1.0f, 0.0f))); },

            // Methods
            "Set",          [](glm::quat& q, float x, float y, float z, float w) { q = glm::quat(w, x, y, z); },
            "Unpack",       [](const glm::quat& q) { return std::make_tuple(q.x, q.y, q.z, q.w); },
            "Clone",        [](const glm::quat& q) { return q; },
            "Normalize",    [](const glm::quat& q) { return glm::normalize(q); },
            "Inverse",      [](const glm::quat& q) { return glm::inverse(q); },
            "Conjugate",    [](const glm::quat& q) { return glm::conjugate(q); },
            "Dot",          [](const glm::quat& a, const glm::quat& b) { return glm::dot(a, b); },
            "Angle",        [](const glm::quat& a, const glm::quat& b) { return 2.0f * glm::acos(glm::min(glm::abs(glm::dot(a, b)), 1.0f)); },
            "Slerp",        [](const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); },
            "Lerp",         [](const glm::quat& a, const glm::quat& b, float t) { return glm::normalize(glm::lerp(a, glm::dot(a, b) < 0.0f ? -b : b, t)); },
            "RotateTowards",[](const glm::quat& from, const glm::quat& to, float maxAngleRad)
                            {
                                const float angle = 2.0f * glm::acos(glm::min(glm::abs(glm::dot(from, to)), 1.0f));
                                return angle <= maxAngleRad || angle == 0.0f ? to : glm::slerp(from, to, maxAngleRad / angle);
                            },
            "Rotate",       [](const glm::quat& q, const glm::vec3& v) { return q * v; },
            "Euler",        [](const glm::quat& q) { return glm::eulerAngles(q); },
            "ToAngleAxis",  [](const glm::quat& q) { return std::make_tuple(glm::angle(q), glm::axis(q)); },
            "ToMat4",       [](const glm::quat& q) { return glm::mat4_cast(q); },
            "Forward",      [](const glm::quat& q) { return q * glm::vec3(0.0f, 0.0f, -1.0f); },
            "Right",        [](const glm::quat& q) { return q * glm::vec3(1.0f, 0.0f, 0.0f); },
            "Up",           [](const glm::quat& q) { return q * glm::vec3(0.0f, 1.0f, 0.0f); }
        );

        // Matrix 4x4 (column major like glm/GLSL: Get(column, row), indices 0-3)
        math.new_usertype<glm::mat4>("Mat4",
            sol::constructors<glm::mat4(), glm::mat4(float)>(), // Mat4.new() is zero, Mat4.new(1.0) identity

            // Operators
            sol::meta_function::multiplication, sol::overload(
                [](const glm::mat4& a, const glm::mat4& b) { return a * b; },
                [](const glm::mat4& m, const glm::vec4& v) { return m * v; }
            ),
            sol::meta_function::equal_to,  [](const glm::mat4& a, const glm::mat4& b) { return a == b; },
            sol::meta_function::to_string, [](const glm::mat4& m)
                                           {
                                               return std::format("Mat4([{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}])",
                                                   m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3],
                                                   m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]);
                                           },

            // Static Generators
            "Identity",     []() { return glm::mat4(1.0f); },
            "TRS",          [](const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
                            {
                                return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
                            },
            "Perspective",  [](float fovYRad, float aspect, float nearClip, float farClip) { return glm::perspective(fovYRad, aspect, nearClip, farClip); },
            "Orthographic", [](float left, float right, float bottom, float top, float nearClip, float farClip) { return glm::ortho(left, right, bottom, top, nearClip, farClip); },
            "LookAt",       [](const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up) { return glm::lookAt(eye, target, up); },

            // Element access (out of range: Get returns nil, Set does nothing)
            "Get",       [](const glm::mat4& m, int column, int row) -> sol::optional<float>
                         {
                             if(column < 0 || column > 3 || row < 0 || row > 3)
                                 return sol::nullopt;
                             return m[column][row];
                         },
            "Set",       [](glm::mat4& m, int column, int row, float value)
                         {
                             if(column >= 0 && column <= 3 && row >= 0 && row <= 3)
                                 m[column][row] = value;
                         },
            "GetColumn", [](const glm::mat4& m, int column) -> sol::optional<glm::vec4>
                         {
                             if(column < 0 || column > 3)
                                 return sol::nullopt;
                             return m[column];
                         },
            "SetColumn", [](glm::mat4& m, int column, const glm::vec4& value)
                         {
                             if(column >= 0 && column <= 3)
                                 m[column] = value;
                         },

            // Methods
            "Translate",          [](const glm::mat4& m, const glm::vec3& v) { return glm::translate(m, v); },
            "Rotate",             [](const glm::mat4& m, float angleRad, const glm::vec3& axis) { return glm::rotate(m, angleRad, axis); },
            "Scale",              [](const glm::mat4& m, const glm::vec3& v) { return glm::scale(m, v); },
            "Inverse",            [](const glm::mat4& m) { return glm::inverse(m); },
            "Transpose",          [](const glm::mat4& m) { return glm::transpose(m); },
            "Determinant",        [](const glm::mat4& m) { return glm::determinant(m); },
            "Clone",              [](const glm::mat4& m) { return m; },
            "TransformPoint",     [](const glm::mat4& m, const glm::vec3& p)
                                  {
                                      const glm::vec4 r = m * glm::vec4(p, 1.0f);
                                      return r.w != 0.0f && r.w != 1.0f ? glm::vec3(r) / r.w : glm::vec3(r);
                                  },
            "TransformDirection", [](const glm::mat4& m, const glm::vec3& d) { return glm::vec3(m * glm::vec4(d, 0.0f)); },
            "Decompose",          [](const glm::mat4& m)
                                  {
                                      glm::vec3 position(0.0f), eulerRad(0.0f), scale(1.0f);
                                      Math::DecomposeTransform(m, position, eulerRad, scale);
                                      return std::make_tuple(position, glm::quat(eulerRad), scale);
                                  }
        );

        // Vector helpers
        math["LerpVec3"] = [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); };
        math["Up"]       = []() { return glm::vec3(0.0f, 1.0f, 0.0f); };
        math["Down"]     = []() { return glm::vec3(0.0f, -1.0f, 0.0f); };
        math["Right"]    = []() { return glm::vec3(1.0f, 0.0f, 0.0f); };
        math["Left"]     = []() { return glm::vec3(-1.0f, 0.0f, 0.0f); };
        math["Forward"]  = []() { return glm::vec3(0.0f, 0.0f, -1.0f); };
        math["Back"]     = []() { return glm::vec3(0.0f, 0.0f, 1.0f); };

        math["RandomInsideUnitCircle"] = []()
        {
            const float angle = sRandom.NextFloat() * glm::two_pi<float>();
            const float radius = glm::sqrt(sRandom.NextFloat());
            return glm::vec2(glm::cos(angle), glm::sin(angle)) * radius;
        };
        math["RandomOnUnitSphere"] = []()
        {
            const float z = sRandom.NextFloat() * 2.0f - 1.0f;
            const float angle = sRandom.NextFloat() * glm::two_pi<float>();
            const float r = glm::sqrt(glm::max(0.0f, 1.0f - z * z));
            return glm::vec3(r * glm::cos(angle), r * glm::sin(angle), z);
        };
        math["RandomInsideUnitSphere"] = []()
        {
            const float z = sRandom.NextFloat() * 2.0f - 1.0f;
            const float angle = sRandom.NextFloat() * glm::two_pi<float>();
            const float r = glm::sqrt(glm::max(0.0f, 1.0f - z * z));
            return glm::vec3(r * glm::cos(angle), r * glm::sin(angle), z) * std::cbrt(sRandom.NextFloat());
        };

        // Colors (HSV: x = hue in degrees 0-360, y = saturation, z = value)
        math["HSVToRGB"]  = [](const glm::vec3& hsv) { return glm::rgbColor(hsv); };
        math["RGBToHSV"]  = [](const glm::vec3& rgb) { return glm::hsvColor(rgb); };
        math["HexToRGB"]  = [](const String& hex) { return glm::vec3(HexToColor(hex)); };
        math["HexToRGBA"] = [](const String& hex) { return HexToColor(hex); };
        math["ColorToHex"] = [](const glm::vec4& color)
        {
            const glm::uvec4 c = glm::uvec4(glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f);
            return std::format("#{:02X}{:02X}{:02X}{:02X}", c.r, c.g, c.b, c.a);
        };
    }
} // namespace Surge::ScriptBinding
