#pragma once

#include <spdlog/common.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace vcmp_lua {

// Settings from luaconfig.lua (plan B7). Phase 1 uses scripts, package_path
// and log.level; log.file, log.daily and http.cafile are only type-checked.
struct Config {
    std::vector<std::string> scripts;
    std::string package_path = "lua/?.lua;lua/?/init.lua";
    spdlog::level::level_enum log_level = spdlog::level::info;
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
