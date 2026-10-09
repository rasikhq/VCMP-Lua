#pragma once

#include <spdlog/common.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace vcmp_lua {

struct LogConfig {
    spdlog::level::level_enum level = spdlog::level::info;
    // Also log to this file, if set. With daily, one file per day:
    // "logs/vcmp-lua.log" becomes "logs/vcmp-lua_2026-10-09.log".
    std::string file;
    bool daily = false;
};

struct HttpConfig {
    // CA certificates (PEM) that the http module trusts instead of the
    // default ones: the system store on Windows; on Linux the distribution's
    // bundle, or the embedded Mozilla bundle if there is none.
    std::string cafile;
};

// Settings from luaconfig.lua (plan B7).
struct Config {
    std::vector<std::string> scripts;
    std::string package_path = "lua/?.lua;lua/?/init.lua";
    LogConfig log;
    HttpConfig http;
};

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Runs the file in a separate Lua state without io, os, package, load or
// dofile, then validates the table it returns. Errors name the field, e.g.
// "luaconfig.lua: scripts[2] must be a string (got number)".
Config LoadConfig(const std::string& path);

}  // namespace vcmp_lua
