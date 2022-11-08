#pragma once
#include "pch.h"
#include <math.h>

static inline void raw_split(const std::string& s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        if (!item.empty())
            elems.push_back(item);
    }
}

namespace std {

    static inline std::vector<std::string> split(const std::string& s, char delim)
    {
        std::vector<std::string> elems;
        raw_split(s, delim, elems);
        return elems;
    }

}

#if defined(_WIN32) || defined(WIN32)
static inline LARGE_INTEGER GetFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency;
}

static int64_t GetCurrentSysTime()
{
    HANDLE current_thread = GetCurrentThread();
    DWORD_PTR previous_mask = SetThreadAffinityMask(current_thread, 1);

    static const LARGE_INTEGER frequency = GetFrequency();

    // Get the current time
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    // Restore the thread affinity
    SetThreadAffinityMask(current_thread, previous_mask);

    // Return the current time as microseconds
    return static_cast<int64_t>(1000000LL * time.QuadPart / frequency.QuadPart);
}
#else
static int64_t GetCurrentSysTime()
{
    // POSIX implementation
    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<int64_t>(uint64_t(time.tv_sec) * 1000000 + time.tv_nsec / 1000);
}
#endif