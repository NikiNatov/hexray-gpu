#pragma once

#include "core/core.h"

class Timer
{
public:
    Timer() = default;

    void Reset();
    void Stop();

    double GetTimeNow() const;
    inline double GetElapsedTimeMS() const { return m_ElapsedTime; }
    inline double GetElapsedTime() const { return m_ElapsedTime / 1000.0; }
private:
    std::chrono::time_point<std::chrono::steady_clock> m_StartPoint;
    double m_ElapsedTime = 0.0;
};