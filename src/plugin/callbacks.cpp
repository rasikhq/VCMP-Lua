// The server callbacks: thin noexcept wrappers around the runtime (plan
// B3.4). No exception crosses into the server's C code, and every callback
// tolerates a missing, closing or dead runtime.
#include <vcmp.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <utility>

#include "plugin/api_guard.hpp"
#include "plugin/plugin.hpp"
#include "runtime/config.hpp"
#include "runtime/libraries.hpp"
#include "runtime/log.hpp"
#include "runtime/runtime.hpp"
#include "runtime/version.hpp"

namespace vcmp_lua::plugin {
namespace {

constexpr const char* kPluginName = "VCMP-Lua";

struct State {
    ServerApi api;
    Options options;
    // Owned here; deleted only by a shutdown or a reload. If the server never
    // calls OnServerShutdown the runtime is leaked on purpose: tearing Lua
    // down from a static destructor or DllMain would run finalizers after
    // the server is gone.
    Runtime* runtime = nullptr;
    // OnServerShutdown arrived while Lua was running; finish once it returns.
    bool shutdown_deferred = false;
};

State& Plugin() {
    static State state;
    return state;
}

// Logs the exception being handled. Call only from a catch block.
void ReportException(const char* where) noexcept {
    try {
        throw;
    } catch (const std::exception& error) {
        log::Error("{}: {}", where, error.what());
    } catch (...) {
        log::Error("{}: unknown exception", where);
    }
}

// Runs body, logging any exception: the noexcept boundary of a callback.
template <typename Body>
void Guarded(const char* where, Body&& body) noexcept {
    try {
        body();
    } catch (...) {
        ReportException(where);
    }
}

// The runtime, unless there is none or it died in a Lua panic. It may be
// closing: pool changes still update its entity pools then.
Runtime* Live() noexcept {
    Runtime* runtime = Plugin().runtime;
    return runtime != nullptr && !runtime->Dead() ? runtime : nullptr;
}

Clock ClockOf(const Options& options) {
    return options.clock ? options.clock : SteadyClock();
}

Config ReadConfig(const Options& options) {
    return options.config ? *options.config : LoadConfig(options.config_path);
}

void FinishShutdown() noexcept {
    State& plugin = Plugin();
    plugin.shutdown_deferred = false;
    if (plugin.runtime == nullptr) {
        return;
    }
    Guarded("OnServerShutdown", [&] { plugin.runtime->NotifyServerShutdown(); });
    Runtime* runtime = std::exchange(plugin.runtime, nullptr);
    runtime->Shutdown(Runtime::ShutdownReason::Server);
    delete runtime;
    plugin.shutdown_deferred = false;  // a handler may have sent another one
    if (plugin.options.manage_libraries) {
        libraries::Cleanup();
    }
    log::Info("Shut down");
}

// Replaces the runtime: the new one reads the config again and runs the
// scripts from scratch. The new runtime is created first (that touches no
// server state), so if the config is now invalid or the Lua state cannot be
// set up, the old runtime keeps running.
void Reload() {
    State& plugin = Plugin();
    std::unique_ptr<Runtime> fresh;
    try {
        Config config = ReadConfig(plugin.options);
        fresh = std::make_unique<Runtime>(config, plugin.api, ClockOf(plugin.options));
        if (plugin.options.on_runtime) {
            plugin.options.on_runtime(*fresh);
        }
        log::Configure(config.log);
    } catch (...) {
        ReportException("Server.reload");
        log::Error("Server.reload: the scripts keep running unchanged");
        plugin.runtime->CancelReload();
        return;
    }
    log::Info("Reloading the scripts");
    // No runtime while the old one shuts down: the server reports the
    // deletion of its entities synchronously, and nobody needs to see them.
    Runtime* old = std::exchange(plugin.runtime, nullptr);
    old->Shutdown(Runtime::ShutdownReason::Reload);
    delete old;
    plugin.runtime = fresh.release();
    plugin.runtime->LoadScripts();
}

// After every callback: a deferred shutdown runs once no call into Lua is
// active anymore (plan B3.1).
void AfterCallback() noexcept {
    State& plugin = Plugin();
    if (plugin.shutdown_deferred && plugin.runtime != nullptr && !plugin.runtime->InLuaCall()) {
        FinishShutdown();
    }
}

// --- Server callbacks ----------------------------------------------------------

uint8_t OnServerInitialise() noexcept {
    Guarded("OnServerInitialise", [] {
        if (Runtime* runtime = Live()) {
            runtime->LoadScripts();
        }
    });
    AfterCallback();
    return 1;
}

void OnServerFrame(float elapsed_seconds) noexcept {
    Guarded("OnServerFrame", [&] {
        if (Runtime* runtime = Live()) {
            runtime->Frame(elapsed_seconds);
        }
    });
    AfterCallback();
    // A reload requested during the frame runs at its end.
    Guarded("Server.reload", [] {
        Runtime* runtime = Live();
        if (runtime != nullptr && runtime->ReloadRequested() && !runtime->InLuaCall() &&
            !Plugin().shutdown_deferred) {
            Reload();
        }
    });
    // The new scripts may have received a shutdown while they loaded.
    AfterCallback();
}

void OnServerShutdown() noexcept {
    State& plugin = Plugin();
    if (plugin.runtime == nullptr) {
        return;
    }
    if (plugin.runtime->InLuaCall()) {
        plugin.shutdown_deferred = true;
        return;
    }
    FinishShutdown();
}

void OnPlayerConnect(int32_t player_id) noexcept {
    Guarded("OnPlayerConnect", [&] {
        Runtime* runtime = Live();
        if (runtime == nullptr) {
            return;
        }
        const PlayerRef player = runtime->Entities().Players().Seen<EntityKind::Player>(player_id);
        if (runtime->Usable()) {
            runtime->Events().Emit(runtime->state(), Event::PlayerConnect, player);
        }
    });
    AfterCallback();
}

void OnPlayerDisconnect(int32_t player_id, vcmpDisconnectReason reason) noexcept {
    Guarded("OnPlayerDisconnect", [&] {
        Runtime* runtime = Live();
        if (runtime == nullptr) {
            return;
        }
        // The player stays valid during the event and is released after it,
        // as in the server (docs/internals.md). The event may arrive inside a
        // binding (KickPlayer); the runtime is still usable then.
        EntityPool& players = runtime->Entities().Players();
        const PlayerRef player = players.Seen<EntityKind::Player>(player_id);
        if (runtime->Usable()) {
            runtime->Events().Emit(runtime->state(), Event::PlayerDisconnect, player, reason);
        }
        // A handler may have reloaded or shut down the runtime: look it up.
        if (Runtime* still = Live()) {
            still->Entities().Players().Release(player_id);
        }
    });
    AfterCallback();
}

void OnEntityPoolChange(vcmpEntityPool type, int32_t entity_id, uint8_t is_deleted) noexcept {
    Guarded("OnEntityPoolChange", [&] {
        Runtime* runtime = Live();
        if (runtime == nullptr) {
            return;
        }
        const bool deleted = is_deleted != 0;
        if (EntityPool* pool = runtime->Entities().FromServerPool(type); pool != nullptr && !deleted) {
            pool->Adopt(entity_id);
        }
        if (runtime->Usable()) {
            runtime->Events().Emit(runtime->state(), Event::EntityPoolChange, type, entity_id,
                                   deleted);
        }
        if (Runtime* still = Live(); still != nullptr && deleted) {
            if (EntityPool* pool = still->Entities().FromServerPool(type)) {
                pool->Release(entity_id);
            }
        }
    });
    AfterCallback();
}

void SetPluginName(PluginInfo* info) noexcept {
    const std::size_t length = std::min(std::strlen(kPluginName), sizeof(info->name) - 1);
    std::memcpy(info->name, kPluginName, length);
    info->name[length] = '\0';
}

// Writes a callback only where the server's struct has room for it.
#define VCMP_LUA_SET_CALLBACK(calls, field, fn)                       \
    do {                                                              \
        if (VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, field)) {      \
            (calls)->field = (fn);                                    \
        } else {                                                      \
            log::Warn("this server version has no " #field " callback"); \
        }                                                             \
    } while (false)

}  // namespace

unsigned int Init(PluginFuncs* funcs, PluginCallbacks* calls, PluginInfo* info,
                  Options options) noexcept {
    const bool manage_libraries = options.manage_libraries;
    try {
        log::Init();
        if (funcs == nullptr || calls == nullptr || info == nullptr) {
            log::Error("VcmpPluginInit: the server passed a null pointer");
            return 0;
        }
        State& plugin = Plugin();
        if (plugin.runtime != nullptr) {
            log::Error("VcmpPluginInit: already initialised");
            return 0;
        }
        if (manage_libraries) {
            libraries::Init();
        }

        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, name)) {
            SetPluginName(info);
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, pluginVersion)) {
            info->pluginVersion = kVersionNumber;
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, apiMajorVersion)) {
            info->apiMajorVersion = PLUGIN_API_MAJOR;
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, apiMinorVersion)) {
            info->apiMinorVersion = PLUGIN_API_MINOR;
        }
        if (!VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerInitialise) ||
            !VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerShutdown) ||
            !VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerFrame)) {
            log::Error("VcmpPluginInit: this server version has no OnServerFrame callback");
            if (manage_libraries) {
                libraries::Cleanup();
            }
            return 0;
        }

        log::Info("VCMP-Lua {} with {}", kVersion, libraries::Versions());
        Config config = ReadConfig(options);
        log::Configure(config.log);

        plugin.api = ServerApi(funcs);
        plugin.options = std::move(options);
        plugin.shutdown_deferred = false;
        auto runtime = std::make_unique<Runtime>(std::move(config), plugin.api, ClockOf(plugin.options));
        if (plugin.options.on_runtime) {
            plugin.options.on_runtime(*runtime);
        }

        // Last: the server must never call into a plugin that failed to start.
        calls->OnServerInitialise = &OnServerInitialise;
        calls->OnServerShutdown = &OnServerShutdown;
        calls->OnServerFrame = &OnServerFrame;
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerConnect, &OnPlayerConnect);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerDisconnect, &OnPlayerDisconnect);
        VCMP_LUA_SET_CALLBACK(calls, OnEntityPoolChange, &OnEntityPoolChange);
        plugin.runtime = runtime.release();
        return 1;
    } catch (...) {
        ReportException("VcmpPluginInit");
        if (manage_libraries) {
            libraries::Cleanup();
        }
        return 0;
    }
}

Runtime* CurrentRuntime() noexcept {
    return Plugin().runtime;
}

}  // namespace vcmp_lua::plugin
