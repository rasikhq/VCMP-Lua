#pragma once

#include <vcmp.h>

#include <functional>
#include <optional>
#include <string>

#include "core/scheduler.hpp"
#include "runtime/config.hpp"

namespace vcmp_lua {

class Runtime;

namespace plugin {

struct Options {
    std::string config_path = "luaconfig.lua";
    // Used instead of reading config_path, on start and on reload (tests).
    std::optional<Config> config;
    // Empty: std::chrono::steady_clock.
    Clock clock;
    // Initialise and clean up the C libraries (libraries.hpp). Only the real
    // plugin does: OpenSSL cannot be initialised again once cleaned up.
    bool manage_libraries = true;
    // Called with every new runtime before its scripts load (tests).
    std::function<void(Runtime&)> on_runtime;
};

// VcmpPluginInit: creates the runtime and installs the server callbacks.
// Returns 1 on success and 0 on failure; never throws (plan B3.4).
//
// The server callbacks (callbacks.cpp) tolerate a missing, closing or dead
// runtime, since the server sends events after OnServerShutdown. A shutdown
// that arrives during a call into Lua is deferred until the call returns, and
// a reload requested by a script runs at the end of the frame.
unsigned int Init(PluginFuncs* funcs, PluginCallbacks* calls, PluginInfo* info,
                  Options options) noexcept;

// The current runtime, or null (tests).
Runtime* CurrentRuntime() noexcept;

}  // namespace plugin
}  // namespace vcmp_lua
