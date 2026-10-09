// Logger: v1's script logging, fixed (A3). v1 compared an unsigned level
// with >= 0 (always true) and showed more the higher the level was set; here
// Logger.setLevel sets the least severe level that is logged, the same
// setting as luaconfig.lua's log.level.
#include <sol/sol.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "bindings/args.hpp"
#include "bindings/bindings.hpp"
#include "runtime/log.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr std::array<std::pair<std::string_view, spdlog::level::level_enum>, 7> kLevels = {{
    {"trace", spdlog::level::trace},
    {"debug", spdlog::level::debug},
    {"info", spdlog::level::info},
    {"warn", spdlog::level::warn},
    {"error", spdlog::level::err},
    {"critical", spdlog::level::critical},
    {"off", spdlog::level::off},
}};

// v1's numbers: 0 debug, 1 info, 2 warn, 3 error, 4 critical; 5 is off.
constexpr std::array<spdlog::level::level_enum, 6> kNumbered = {
    spdlog::level::debug, spdlog::level::info,     spdlog::level::warn,
    spdlog::level::err,   spdlog::level::critical, spdlog::level::off,
};

spdlog::level::level_enum CheckLevel(lua_State* L, int index) {
    if (lua_type(L, index) == LUA_TNUMBER) {
        return kNumbered[static_cast<std::size_t>(CheckInteger(L, index, 0, 5))];
    }
    const std::string name = CheckString(L, index);
    for (const auto& [level_name, level] : kLevels) {
        if (name == level_name) {
            return level;
        }
    }
    ArgError(L, index, "level must be trace, debug, info, warn, error, critical, off or 0-5");
}

template <spdlog::level::level_enum Level>
void Write(String message) {
    log::Write(Level, "{}", message.value);
}

}  // namespace

void RegisterLogger(sol::state& lua) {
    sol::table logger = lua.create_named_table("Logger");
    logger["debug"] = &Write<spdlog::level::debug>;
    logger["info"] = &Write<spdlog::level::info>;
    logger["warn"] = &Write<spdlog::level::warn>;
    logger["error"] = &Write<spdlog::level::err>;
    logger["critical"] = &Write<spdlog::level::critical>;

    // Logger.setLevel(level): "debug", "info", ... or v1's 0 (debug) to 4
    // (critical); messages below it are not logged.
    logger["setLevel"] = [](sol::this_state L) { log::SetLevel(CheckLevel(L, 1)); };
    // Logger.getLevel(): the current level's name.
    logger["getLevel"] = []() -> std::string_view {
        const spdlog::logger* current = log::Logger();
        const auto level = current != nullptr ? current->level() : spdlog::level::info;
        for (const auto& [name, value] : kLevels) {
            if (value == level) {
                return name;
            }
        }
        return "info";
    };
}

}  // namespace vcmp_lua::bindings
