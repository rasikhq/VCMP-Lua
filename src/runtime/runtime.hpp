#pragma once

#include <memory>
#include <optional>

#include <sol/sol.hpp>

#include "core/entity_pool.hpp"
#include "core/event_bus.hpp"
#include "core/frame_pump.hpp"
#include "core/invoker.hpp"
#include "core/scheduler.hpp"
#include "modules/http.hpp"
#include "plugin/api_guard.hpp"
#include "runtime/config.hpp"

namespace vcmp_lua {

// Owns the Lua state and everything that refers into it (plan B3.1). Created
// in VcmpPluginInit (or by a reload), scripts run in OnServerInitialise, and
// Shutdown() tears everything down in a fixed order. Only the server's main
// thread uses it.
class Runtime {
public:
    enum class ShutdownReason { Server, Reload };

    Runtime(Config config, ServerApi api, Clock clock);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    // OnServerInitialise (or the end of a reload): adopts the players and
    // entities that exist, runs the configured scripts in order, then emits
    // onServerInit.
    void LoadScripts();

    // OnServerFrame: the frame pump (timers), then onServerFrame.
    void Frame(float elapsed_seconds);

    // OnServerShutdown, before Shutdown(): emits onServerShutdown, then
    // onPlayerDisconnect for every player still online. The server reports
    // those disconnects only after the runtime has closed (docs/internals.md),
    // so the runtime reports them itself, with the reason the server uses
    // (vcmpDisconnectReasonTimeout).
    void NotifyServerShutdown();

    // 1. Mark the runtime closing; on reload, delete the entities it created.
    // 2. Release the Lua references C++ holds. 3. Close the Lua state, which
    // runs __gc and __close while the subsystems still exist. 4. Destroy the
    // subsystems. Idempotent. Call only when InLuaCall() is false.
    void Shutdown(ShutdownReason reason) noexcept;

    // False once closing, shut down, or dead after a Lua panic.
    [[nodiscard]] bool Usable() const noexcept;

    // True while C++ is inside a call into Lua. Shutdown and reload requests
    // that arrive then are deferred until the call returns.
    [[nodiscard]] bool InLuaCall() const noexcept;

    // Called by the panic handler. The Lua state is never touched again.
    void MarkDead() noexcept;
    [[nodiscard]] bool Dead() const noexcept { return dead_; }

    // Server.reload(): the plugin replaces this runtime at the end of the
    // frame, once no call into Lua is active.
    void RequestReload() noexcept { reload_requested_ = true; }
    [[nodiscard]] bool ReloadRequested() const noexcept { return reload_requested_; }
    void CancelReload() noexcept { reload_requested_ = false; }

    // The runtime that owns L, or null.
    static Runtime* FromState(lua_State* L) noexcept;

    // The runtime that owns L; throws "runtime shutting down" unless usable.
    // Every binding starts with it.
    static Runtime& Require(lua_State* L);

    [[nodiscard]] lua_State* state() const noexcept { return lua_->lua_state(); }
    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] const ServerApi& api() const noexcept { return api_; }
    Invoker& invoker() noexcept { return *invoker_; }
    EventBus& Events() noexcept { return *events_; }
    Scheduler& Timers() noexcept { return *timers_; }
    EntityPools& Entities() noexcept { return *entities_; }
    FramePump& Pump() noexcept { return *pump_; }
    Http& Requests() noexcept { return *http_; }

    // What a dead runtime leaves behind (runtime.cpp).
    struct Remains;

private:
    Config config_;
    ServerApi api_;
    std::unique_ptr<Remains> remains_;
    // Declared first, destroyed last: the members below hold references into it.
    std::optional<sol::state> lua_;
    std::unique_ptr<Invoker> invoker_;
    std::unique_ptr<EntityPools> entities_;
    std::unique_ptr<EventBus> events_;
    std::unique_ptr<Scheduler> timers_;
    std::unique_ptr<Http> http_;
    std::unique_ptr<FramePump> pump_;
    bool closing_ = false;
    bool dead_ = false;
    bool reload_requested_ = false;
};

}  // namespace vcmp_lua
