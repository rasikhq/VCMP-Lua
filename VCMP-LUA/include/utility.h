#pragma once
#include "pch.h"

static inline void OutputErrorImpl(const char* msg, va_list args)
{
#if defined(WIN32) || defined(_WIN32)
    HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csb_before;
    GetConsoleScreenBufferInfo(hstdout, &csb_before);
    SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY); // NOLINT(hicpp-signed-bitwise)
    std::printf("[LUA] ");

    SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY); // NOLINT(hicpp-signed-bitwise)
    std::vprintf(msg, args);
    std::puts("");

    SetConsoleTextAttribute(hstdout, csb_before.wAttributes);
#else
    std::printf("\033[21;91m[LUA]\033[0m");
    std::vprintf(msg, args);
    std::puts("");
#endif // WIN32
}
static inline void OutputError(const char* msg, ...) { 
    va_list args;
    va_start(args, msg);
    OutputErrorImpl(msg, args);
    va_end(args);
}

static inline void OutputErrorCallbackCall(const char* callbackName) {
    OutputError(std::string("[ERROR] Failed to call: " + std::string(callbackName)).c_str());
}

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