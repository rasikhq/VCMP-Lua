#pragma once

#include <optional>

#include <sol/sol.hpp>

#include "runtime/config.hpp"

namespace vcmp_lua {

// Owns the Lua state and everything that refers into it (plan B3.1). Created
// in VcmpPluginInit, scripts run in OnServerInitialise, and Shutdown() tears
// everything down in a fixed order. Only the server's main thread uses it.
//
// Phase 1 skeleton: the event bus, scheduler, entity pools and HTTP pump
// arrive in later phases and slot into Shutdown() in the documented order.
class Runtime {
public:
    explicit Runtime(Config config);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    // OnServerInitialise: runs the configured scripts, in order.
    void LoadScripts();

    // OnServerFrame.
    void Frame(float elapsed_seconds);

    // 1. Mark the runtime closing. 2. Release the Lua references C++ holds.
    // 3. Close the Lua state, which runs __gc and __close while the
    // subsystems still exist. 4. Destroy the subsystems. Idempotent.
    void Shutdown() noexcept;

    // False once closing, shut down, or dead after a Lua panic.
    [[nodiscard]] bool Usable() const noexcept;

    // True while C++ is inside a call into Lua. Shutdown requests that arrive
    // then are deferred until the call returns.
    [[nodiscard]] bool InLuaCall() const noexcept { return call_depth_ > 0; }

    // Called by the panic handler. The Lua state is never touched again.
    void MarkDead() noexcept { dead_ = true; }

    // The runtime that owns L, or null.
    static Runtime* FromState(lua_State* L) noexcept;

private:
    class CallScope;

    Config config_;
    std::optional<sol::state> lua_;
    int call_depth_ = 0;
    bool closing_ = false;
    bool dead_ = false;
};

}  // namespace vcmp_lua
