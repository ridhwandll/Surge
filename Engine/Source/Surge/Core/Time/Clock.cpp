// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Clock.hpp"
#include "Pch.hpp"
#include <chrono>

namespace Surge
{
    static std::chrono::high_resolution_clock::time_point sStart;
    static std::chrono::high_resolution_clock::time_point sThen;
    static std::chrono::high_resolution_clock::time_point sNow;

    void Clock::Start()
    {
        sStart = std::chrono::high_resolution_clock::now();
        sThen = sStart;
        sNow = sStart;
    }

    float Clock::GetLife()
    {
        std::chrono::duration<float> lifeTime = std::chrono::duration_cast<std::chrono::duration<float>>(sNow - sStart);
        return lifeTime.count();
    }

    float Clock::GetDelta()
    {
        std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(sNow - sThen);
        return deltaTime.count();
    }

    void Clock::Update()
    {
        sThen = sNow;
        sNow = std::chrono::high_resolution_clock::now();
    }
} // namespace Surge