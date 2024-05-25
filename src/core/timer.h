#pragma once

#include "core/core.h"

class Timer
{
public:
    Timer() = default;

    void Reset();
    void Stop();

    inline float GetElapsedTimeMS() const { return m_ElapsedTime; }
    inline float GetElapsedTime() const { return m_ElapsedTime / 1000.0f; }
private:
    std::chrono::time_point<std::chrono::steady_clock> m_StartPoint;
    float m_ElapsedTime = 0.0f;
};