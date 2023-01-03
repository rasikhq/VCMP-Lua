#pragma once
#include <pch.h>
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

static uint32_t GetCurrentSysTime()
{
    // POSIX implementation
    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<uint32_t>(uint64_t(time.tv_sec) * 1000000 + time.tv_nsec / 1000);
}