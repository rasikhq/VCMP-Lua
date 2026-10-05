#include "runtime/log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

namespace vcmp_lua::log {
namespace {

// Leaked on purpose: no static destructor may run while the server can still
// call into the plugin.
spdlog::logger* g_logger = nullptr;

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

}  // namespace vcmp_lua::log
