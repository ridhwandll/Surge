// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <chrono>

namespace Surge
{
    class Timer
    {
    public:
        Timer()
        {
            Reset();
        }

        void Reset()
        {
            mStart = std::chrono::high_resolution_clock::now();
        }

        // Returns the elapsed time in seconds
        float Elapsed()
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - mStart).count() * 0.001f * 0.001f * 0.001f;
        }

        float ElapsedMillis()
        {
            return Elapsed() * 1000.0f;
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
    };
}