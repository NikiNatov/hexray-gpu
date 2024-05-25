#include "timer.h"

// ------------------------------------------------------------------------------------------------------------------------------------
void Timer::Reset()
{
    m_StartPoint = std::chrono::steady_clock::now();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Timer::Stop()
{
    auto endPoint = std::chrono::steady_clock::now();
    m_ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endPoint - m_StartPoint).count();
}
