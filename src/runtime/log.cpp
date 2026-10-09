#include "runtime/log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <vector>

namespace vcmp_lua::log {
namespace {

// Leaked on purpose: no static destructor may run while the server can still
// call into the plugin.
spdlog::logger* g_logger = nullptr;
// The file sink added by Configure, if any. Owned by the logger's sink list.
spdlog::sinks::sink* g_file_sink = nullptr;

}  // namespace

void Init() {
    if (g_logger != nullptr) {
        return;
    }
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto* logger = new spdlog::logger("vcmp-lua", std::move(sink));
    logger->set_pattern("[%H:%M:%S] [VCMP-Lua] [%^%l%$] %v");
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::trace);
    g_logger = logger;
}

spdlog::logger* Logger() noexcept {
    return g_logger;
}

void SetLevel(spdlog::level::level_enum level) noexcept {
    if (g_logger != nullptr) {
        g_logger->set_level(level);
    }
}

void Configure(const LogConfig& config) noexcept {
    if (g_logger == nullptr) {
        return;
    }
    g_logger->set_level(config.level);
    std::vector<spdlog::sink_ptr>& sinks = g_logger->sinks();
    if (g_file_sink != nullptr) {
        std::erase_if(sinks, [](const spdlog::sink_ptr& sink) { return sink.get() == g_file_sink; });
        g_file_sink = nullptr;
    }
    if (config.file.empty()) {
        return;
    }
    try {
        spdlog::sink_ptr sink;
        if (config.daily) {
            sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(config.file, 0, 0);
        } else {
            sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.file);
        }
        sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
        g_file_sink = sink.get();
        sinks.push_back(std::move(sink));
    } catch (const std::exception& error) {
        Error("cannot open the log file {}: {}", config.file, error.what());
    }
}

}  // namespace vcmp_lua::log
