#include "runtime/config.hpp"

#include <fmt/format.h>
#include <lua.hpp>

#include <array>
#include <string_view>
#include <utility>

#include "runtime/errors.hpp"
#include "runtime/log.hpp"

namespace vcmp_lua {
namespace {

// A config file may run at most this many VM instructions.
constexpr int kInstructionBudget = 10'000'000;

// Owns the sandbox state. A state that panicked is leaked, not closed.
class SandboxState {
public:
    SandboxState() : L_(luaL_newstate()) {
        if (L_ == nullptr) {
            throw ConfigError("cannot create a Lua state for the config file");
        }
        *static_cast<void**>(lua_getextraspace(L_)) = nullptr;  // not a Runtime
        lua_atpanic(L_, &Panic);
    }
    ~SandboxState() {
        if (!panicked_) {
            lua_close(L_);
        }
    }
    SandboxState(const SandboxState&) = delete;
    SandboxState& operator=(const SandboxState&) = delete;

    lua_State* get() const noexcept { return L_; }
    void MarkPanicked() noexcept { panicked_ = true; }

private:
    lua_State* L_;
    bool panicked_ = false;
};

// The lua_CFunctions below run inside lua_pcall; they own no C++ objects.

void BudgetHook(lua_State* L, lua_Debug*) {
    luaL_error(L, "the config file runs too long (endless loop?)");
}

int OpenSandbox(lua_State* L) {
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_settop(L, 0);
    // Base functions that reach files or run other code.
    static constexpr std::array<const char*, 4> kRemoved = {"dofile", "loadfile", "load",
                                                            "collectgarbage"};
    for (const char* name : kRemoved) {
        lua_pushnil(L);
        lua_setglobal(L, name);
    }
    return 0;
}

// Arg 1: light userdata, NUL-terminated path. Returns what the file returns.
int RunConfigFile(lua_State* L) {
    const auto* path = static_cast<const char*>(lua_touserdata(L, 1));
    if (luaL_loadfilex(L, path, "t") != LUA_OK) {
        return lua_error(L);
    }
    lua_sethook(L, &BudgetHook, LUA_MASKCOUNT, kInstructionBudget);
    lua_call(L, 0, 1);
    lua_sethook(L, nullptr, 0, 0);
    return 1;
}

// Validation reads the result with raw accesses only, so no metamethod runs.
class Reader {
public:
    Reader(lua_State* L, std::string file_name) : L_(L), file_(std::move(file_name)) {}

    [[noreturn]] void Fail(std::string_view field, std::string_view expected, int idx) const {
        throw ConfigError(fmt::format("{}: {} must be {} (got {})", file_, field, expected,
                                      luaL_typename(L_, idx)));
    }

    std::string String(int idx, std::string_view field) const {
        if (lua_type(L_, idx) != LUA_TSTRING) {
            Fail(field, "a string", idx);
        }
        std::size_t length = 0;
        const char* text = lua_tolstring(L_, idx, &length);
        return std::string(text, length);
    }

    bool Boolean(int idx, std::string_view field) const {
        if (lua_type(L_, idx) != LUA_TBOOLEAN) {
            Fail(field, "a boolean", idx);
        }
        return lua_toboolean(L_, idx) != 0;
    }

    std::vector<std::string> StringList(int idx, std::string_view field) const {
        if (lua_type(L_, idx) != LUA_TTABLE) {
            Fail(field, "a list of strings", idx);
        }
        std::vector<std::string> items;
        const lua_Unsigned count = lua_rawlen(L_, idx);
        for (lua_Unsigned i = 1; i <= count; ++i) {
            lua_rawgeti(L_, idx, static_cast<lua_Integer>(i));
            items.push_back(String(lua_gettop(L_), fmt::format("{}[{}]", field, i)));
            lua_pop(L_, 1);
        }
        return items;
    }

    // Calls visit(key, value_index) for each string key of the table at idx.
    template <typename Visit>
    void Fields(int idx, std::string_view field, Visit&& visit) const {
        if (lua_type(L_, idx) != LUA_TTABLE) {
            Fail(field, "a table", idx);
        }
        lua_pushnil(L_);
        while (lua_next(L_, idx) != 0) {
            const int value = lua_gettop(L_);
            if (lua_type(L_, -2) == LUA_TSTRING) {
                visit(std::string_view(lua_tostring(L_, -2)), value);
            } else {
                log::Warn("{}: {} has a key that is not a string; it is ignored", file_, field);
            }
            lua_settop(L_, value - 1);
        }
    }

    void Unknown(std::string_view field) const {
        log::Warn("{}: unknown setting '{}' is ignored", file_, field);
    }

    const std::string& file() const noexcept { return file_; }

private:
    lua_State* L_;
    std::string file_;
};

spdlog::level::level_enum ParseLevel(const Reader& reader, int idx) {
    static constexpr std::array<std::pair<std::string_view, spdlog::level::level_enum>, 7> kLevels = {{
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"critical", spdlog::level::critical},
        {"off", spdlog::level::off},
    }};
    const std::string name = reader.String(idx, "log.level");
    for (const auto& [text, level] : kLevels) {
        if (name == text) {
            return level;
        }
    }
    throw ConfigError(fmt::format(
        "{}: log.level must be one of trace, debug, info, warn, error, critical, off (got \"{}\")",
        reader.file(), name));
}

Config ReadConfig(lua_State* L, const std::string& file) {
    const Reader reader(L, file);
    Config config;
    reader.Fields(lua_gettop(L), "the returned table", [&](std::string_view key, int value) {
        if (key == "scripts") {
            config.scripts = reader.StringList(value, "scripts");
        } else if (key == "package_path") {
            config.package_path = reader.String(value, "package_path");
        } else if (key == "log") {
            reader.Fields(value, "log", [&](std::string_view log_key, int log_value) {
                if (log_key == "level") {
                    config.log.level = ParseLevel(reader, log_value);
                } else if (log_key == "file") {
                    config.log.file = reader.String(log_value, "log.file");
                } else if (log_key == "daily") {
                    config.log.daily = reader.Boolean(log_value, "log.daily");
                } else {
                    reader.Unknown(fmt::format("log.{}", log_key));
                }
            });
        } else if (key == "http") {
            reader.Fields(value, "http", [&](std::string_view http_key, int http_value) {
                if (http_key == "cafile") {
                    reader.String(http_value, "http.cafile");
                } else {
                    reader.Unknown(fmt::format("http.{}", http_key));
                }
            });
        } else {
            reader.Unknown(key);
        }
    });
    return config;
}

}  // namespace

Config LoadConfig(const std::string& path) {
    SandboxState state;
    lua_State* L = state.get();
    try {
        lua_pushcfunction(L, &OpenSandbox);
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            throw ConfigError("cannot set up the config sandbox: " + ErrorText(L, -1));
        }
        lua_pushcfunction(L, &RunConfigFile);
        lua_pushlightuserdata(L, const_cast<char*>(path.c_str()));
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            throw ConfigError(ErrorText(L, -1));
        }
        if (lua_type(L, -1) != LUA_TTABLE) {
            throw ConfigError(
                fmt::format("{} must return a table (got {})", path, luaL_typename(L, -1)));
        }
        return ReadConfig(L, path);
    } catch (const LuaPanic& panic) {
        state.MarkPanicked();
        throw ConfigError(fmt::format("{}: {}", path, panic.what()));
    }
}

}  // namespace vcmp_lua
