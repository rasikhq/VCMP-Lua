#pragma once

#include <spdlog/logger.h>

#include <utility>

#include "runtime/config.hpp"

namespace vcmp_lua::log {

// Creates the process-wide logger (console). Call once, first thing in
// VcmpPluginInit. The logger is never destroyed, so logging keeps working in
// callbacks that arrive after shutdown.
void Init();

// Null before Init().
spdlog::logger* Logger() noexcept;

void SetLevel(spdlog::level::level_enum level) noexcept;

// Applies luaconfig.lua's log settings: the level, and the log file, which
// replaces the previous one (a reload may change it). A file that cannot be
// opened is reported, and logging continues on the console.
void Configure(const LogConfig& config) noexcept;

// Logging never throws: callers include noexcept server callbacks.
template <typename... Args>
void Write(spdlog::level::level_enum level, spdlog::format_string_t<Args...> format,
           Args&&... args) noexcept {
    try {
        if (spdlog::logger* logger = Logger()) {
            logger->log(level, format, std::forward<Args>(args)...);
        }
    } catch (...) {
        // Nothing sensible is left to report to.
    }
}

template <typename... Args>
void Debug(spdlog::format_string_t<Args...> format, Args&&... args) noexcept {
    Write(spdlog::level::debug, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(spdlog::format_string_t<Args...> format, Args&&... args) noexcept {
    Write(spdlog::level::info, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Warn(spdlog::format_string_t<Args...> format, Args&&... args) noexcept {
    Write(spdlog::level::warn, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(spdlog::format_string_t<Args...> format, Args&&... args) noexcept {
    Write(spdlog::level::err, format, std::forward<Args>(args)...);
}

}  // namespace vcmp_lua::log
