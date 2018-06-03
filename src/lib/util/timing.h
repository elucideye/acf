/*! -*-c++-*-
  @file   timing.h
  @author Shervin Emami
  @brief  Common timer macros

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_timing_h__
#define __acf_timing_h__

#include <util/acf_util.h>

#include <chrono>
#include <functional>

UTIL_NAMESPACE_BEGIN

class ScopeTimeLogger
{
    using HighResolutionClock = std::chrono::high_resolution_clock;
    using TimePoint = HighResolutionClock::time_point;

public:
    template <class Callable>
    ScopeTimeLogger(Callable&& logger)
        : m_logger(std::forward<Callable>(logger))
    {
        m_tic = HighResolutionClock::now();
    }

    ScopeTimeLogger(ScopeTimeLogger&& other)
        : m_logger(std::move(other.m_logger))
        , m_tic(other.m_tic)
    {
    }

    ~ScopeTimeLogger()
    {
        auto now = HighResolutionClock::now();
        m_logger(timeDifference(now, m_tic));
    }

    ScopeTimeLogger(const ScopeTimeLogger&) = delete;
    void operator=(const ScopeTimeLogger&) = delete;

    static double timeDifference(const TimePoint& a, const TimePoint& b)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(a - b).count();
    }

    const TimePoint& getTime() const { return m_tic; }

protected:
    std::function<void(double)> m_logger;
    TimePoint m_tic;
};

UTIL_NAMESPACE_END

#endif // __acf_timing_h__
