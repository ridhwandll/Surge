// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Clock.hpp"

namespace Surge
{
    void Clock::Start()
    {
        mStart = std::chrono::high_resolution_clock::now();
        mThen = mStart;
        mNow = mStart;
    }

    float Clock::GetLife()
    {
        std::chrono::duration<float> lifeTime = std::chrono::duration_cast<std::chrono::duration<float>>(mNow - mStart);
        return lifeTime.count();
    }

    float Clock::GetSeconds()
    {
        std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(mNow - mThen);
        return deltaTime.count();
    }

    float Clock::GetMilliseconds()
    {
        std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(mNow - mThen);
        return deltaTime.count() * 1000.0f;
    }

    void Clock::Update()
    {
        mThen = mNow;
        mNow = std::chrono::high_resolution_clock::now();
    }

} // namespace Surge
