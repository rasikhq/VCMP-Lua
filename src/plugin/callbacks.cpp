// The server callbacks: thin noexcept wrappers around the runtime. No
// exception crosses into the server's C code, and every callback tolerates a
// missing, closing or dead runtime.
#include <vcmp.h>
#include <sol/sol.hpp>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bindings/stream.hpp"
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
constexpr uint16_t kApiMinorVersion = 0;

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
// active anymore.
void AfterCallback() noexcept {
    State& plugin = Plugin();
    if (plugin.shutdown_deferred && plugin.runtime != nullptr && !plugin.runtime->InLuaCall()) {
        FinishShutdown();
    }
}

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
        if (EntityPool* pool = runtime->Entities().FromServerPool(type);
            pool != nullptr && !deleted) {
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

// The runtime an event may be dispatched to: live and not closing.
Runtime* Dispatchable() noexcept {
    Runtime* runtime = Live();
    return runtime != nullptr && runtime->Usable() ? runtime : nullptr;
}

// Runs body(runtime) for an event callback and returns what the server
// expects from a cancellable one: 0 when a handler called Event.cancel().
// body returns true when the event was cancelled.
template <typename Body>
uint8_t OnEvent(const char* where, Body&& body) noexcept {
    bool cancelled = false;
    Guarded(where, [&] {
        if (Runtime* runtime = Dispatchable()) {
            cancelled = body(*runtime);
        }
    });
    AfterCallback();
    return cancelled ? 0 : 1;
}

// The handle of an entity the server reports, adopted on first sight; none
// for -1 or any id outside the pool.
template <EntityKind K>
EntityRef<K> Seen(Runtime& runtime, int32_t id) {
    EntityPool& pool = runtime.Entities().Get(K);
    if (!pool.InRange(id)) {
        return {};
    }
    return pool.Seen<K>(id);
}

PlayerRef SeenPlayer(Runtime& runtime, int32_t id) {
    return Seen<EntityKind::Player>(runtime, id);
}

template <typename... Args>
bool Emit(Runtime& runtime, Event event, const Args&... args) {
    return runtime.Events().Emit(runtime.state(), event, args...);
}

uint8_t OnIncomingConnection(char* name, size_t name_size, const char* password,
                             const char* ip) noexcept {
    return OnEvent("OnIncomingConnection", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerConnection, name, name_size, password, ip);
    });
}

void OnPlayerModuleList(int32_t player_id, const char* list) noexcept {
    OnEvent("OnPlayerModuleList", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerModuleList, SeenPlayer(runtime, player_id), list);
    });
}

uint8_t OnPlayerRequestClass(int32_t player_id, int32_t offset) noexcept {
    return OnEvent("OnPlayerRequestClass", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerRequestClass, SeenPlayer(runtime, player_id), offset);
    });
}

uint8_t OnPlayerRequestSpawn(int32_t player_id) noexcept {
    return OnEvent("OnPlayerRequestSpawn", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerRequestSpawn, SeenPlayer(runtime, player_id));
    });
}

void OnPlayerSpawn(int32_t player_id) noexcept {
    OnEvent("OnPlayerSpawn", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerSpawn, SeenPlayer(runtime, player_id));
    });
}

// onPlayerKill(killer, player, reason, bodyPart) when another player killed
// the player, else onPlayerWasted(player, reason) with v1's reasons.
void OnPlayerDeath(int32_t player_id, int32_t killer_id, int32_t reason,
                   vcmpBodyPart body_part) noexcept {
    OnEvent("OnPlayerDeath", [&](Runtime& runtime) {
        const PlayerRef player = SeenPlayer(runtime, player_id);
        const auto connected = VCMP_LUA_FIND(runtime.api(), IsPlayerConnected);
        if (killer_id >= 0 && connected != nullptr && connected(killer_id) != 0) {
            return Emit(runtime, Event::PlayerKill, SeenPlayer(runtime, killer_id), player, reason,
                        static_cast<int32_t>(body_part));
        }
        if (reason == 43 || reason == 50) {
            reason = 43;  // drowned
        } else if (reason == 39 && body_part == vcmpBodyPartInVehicle) {
            reason = 39;  // car crash
        } else if (reason == 39 || reason == 40 || reason == 44) {
            reason = 44;  // fell
        }
        return Emit(runtime, Event::PlayerWasted, player, reason);
    });
}

void OnPlayerUpdate(int32_t player_id, vcmpPlayerUpdate update) noexcept {
    OnEvent("OnPlayerUpdate", [&](Runtime& runtime) {
        if (!runtime.Events().HasHandlers(EventBus::Index(Event::PlayerUpdate))) {
            return false;  // many per second: skip the adoption too
        }
        return Emit(runtime, Event::PlayerUpdate, SeenPlayer(runtime, player_id),
                    static_cast<int32_t>(update));
    });
}

uint8_t OnPlayerRequestEnterVehicle(int32_t player_id, int32_t vehicle_id, int32_t slot) noexcept {
    return OnEvent("OnPlayerRequestEnterVehicle", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerRequestEnterVehicle, SeenPlayer(runtime, player_id),
                    Seen<EntityKind::Vehicle>(runtime, vehicle_id), slot);
    });
}

void OnPlayerEnterVehicle(int32_t player_id, int32_t vehicle_id, int32_t slot) noexcept {
    OnEvent("OnPlayerEnterVehicle", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerEnterVehicle, SeenPlayer(runtime, player_id),
                    Seen<EntityKind::Vehicle>(runtime, vehicle_id), slot);
    });
}

void OnPlayerExitVehicle(int32_t player_id, int32_t vehicle_id) noexcept {
    OnEvent("OnPlayerExitVehicle", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerExitVehicle, SeenPlayer(runtime, player_id),
                    Seen<EntityKind::Vehicle>(runtime, vehicle_id));
    });
}

void OnPlayerNameChange(int32_t player_id, const char* old_name, const char* new_name) noexcept {
    OnEvent("OnPlayerNameChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerNameChange, SeenPlayer(runtime, player_id), old_name,
                    new_name);
    });
}

void OnPlayerStateChange(int32_t player_id, vcmpPlayerState old_state,
                         vcmpPlayerState new_state) noexcept {
    OnEvent("OnPlayerStateChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerStateChange, SeenPlayer(runtime, player_id),
                    static_cast<int32_t>(old_state), static_cast<int32_t>(new_state));
    });
}

void OnPlayerActionChange(int32_t player_id, int32_t old_action, int32_t new_action) noexcept {
    OnEvent("OnPlayerActionChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerActionChange, SeenPlayer(runtime, player_id), old_action,
                    new_action);
    });
}

void OnPlayerOnFireChange(int32_t player_id, uint8_t on_fire) noexcept {
    OnEvent("OnPlayerOnFireChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerFireChange, SeenPlayer(runtime, player_id), on_fire != 0);
    });
}

void OnPlayerCrouchChange(int32_t player_id, uint8_t crouching) noexcept {
    OnEvent("OnPlayerCrouchChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerCrouchChange, SeenPlayer(runtime, player_id),
                    crouching != 0);
    });
}

void OnPlayerGameKeysChange(int32_t player_id, uint32_t old_keys, uint32_t new_keys) noexcept {
    OnEvent("OnPlayerGameKeysChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerGameKeysChange, SeenPlayer(runtime, player_id), old_keys,
                    new_keys);
    });
}

void OnPlayerBeginTyping(int32_t player_id) noexcept {
    OnEvent("OnPlayerBeginTyping", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerBeginTyping, SeenPlayer(runtime, player_id));
    });
}

void OnPlayerEndTyping(int32_t player_id) noexcept {
    OnEvent("OnPlayerEndTyping", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerFinishTyping, SeenPlayer(runtime, player_id));
    });
}

void OnPlayerAwayChange(int32_t player_id, uint8_t away) noexcept {
    OnEvent("OnPlayerAwayChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerAwayChange, SeenPlayer(runtime, player_id), away != 0);
    });
}

uint8_t OnPlayerMessage(int32_t player_id, const char* message) noexcept {
    return OnEvent("OnPlayerMessage", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerMessage, SeenPlayer(runtime, player_id), message);
    });
}

// onPlayerCommand(player, command, args, text), as in v1: "/give 5 100"
// gives "give", {"5", "100"}; args is nil without arguments, and command is
// nil for an empty message. text is everything after the command ("5 100"),
// for commands whose argument contains spaces.
uint8_t OnPlayerCommand(int32_t player_id, const char* message) noexcept {
    return OnEvent("OnPlayerCommand", [&](Runtime& runtime) {
        const std::string_view text = message != nullptr ? message : "";
        std::vector<std::string_view> words;
        std::size_t rest = text.size();
        for (std::size_t pos = 0; pos < text.size();) {
            const std::size_t start = text.find_first_not_of(' ', pos);
            if (start == std::string_view::npos) {
                break;
            }
            const std::size_t end = std::min(text.find(' ', start), text.size());
            if (words.size() == 1) {
                rest = start;
            }
            words.push_back(text.substr(start, end - start));
            pos = end;
        }
        const PlayerRef player = SeenPlayer(runtime, player_id);
        if (words.empty()) {
            return Emit(runtime, Event::PlayerCommand, player, sol::lua_nil, sol::lua_nil,
                        std::string_view());
        }
        const std::string_view arguments = text.substr(std::min(rest, text.size()));
        if (words.size() == 1) {
            return Emit(runtime, Event::PlayerCommand, player, words[0], sol::lua_nil, arguments);
        }
        sol::main_table args(runtime.state(), sol::create);
        for (std::size_t i = 1; i < words.size(); ++i) {
            args.raw_set(static_cast<int>(i), words[i]);
        }
        return Emit(runtime, Event::PlayerCommand, player, words[0], args, arguments);
    });
}

uint8_t OnPlayerPrivateMessage(int32_t player_id, int32_t target_id, const char* message) noexcept {
    return OnEvent("OnPlayerPrivateMessage", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerPM, SeenPlayer(runtime, player_id),
                    SeenPlayer(runtime, target_id), message);
    });
}

void OnPlayerSpectate(int32_t player_id, int32_t target_id) noexcept {
    OnEvent("OnPlayerSpectate", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerSpectate, SeenPlayer(runtime, player_id),
                    SeenPlayer(runtime, target_id));
    });
}

void OnPlayerCrashReport(int32_t player_id, const char* report) noexcept {
    OnEvent("OnPlayerCrashReport", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerCrashReport, SeenPlayer(runtime, player_id), report);
    });
}

void OnVehicleUpdate(int32_t vehicle_id, vcmpVehicleUpdate update) noexcept {
    OnEvent("OnVehicleUpdate", [&](Runtime& runtime) {
        if (!runtime.Events().HasHandlers(EventBus::Index(Event::VehicleUpdate))) {
            return false;  // many per second: skip the adoption too
        }
        return Emit(runtime, Event::VehicleUpdate, Seen<EntityKind::Vehicle>(runtime, vehicle_id),
                    static_cast<int32_t>(update));
    });
}

void OnVehicleExplode(int32_t vehicle_id) noexcept {
    OnEvent("OnVehicleExplode", [&](Runtime& runtime) {
        return Emit(runtime, Event::VehicleExplode, Seen<EntityKind::Vehicle>(runtime, vehicle_id));
    });
}

void OnVehicleRespawn(int32_t vehicle_id) noexcept {
    OnEvent("OnVehicleRespawn", [&](Runtime& runtime) {
        return Emit(runtime, Event::VehicleRespawn, Seen<EntityKind::Vehicle>(runtime, vehicle_id));
    });
}

void OnObjectShot(int32_t object_id, int32_t player_id, int32_t weapon) noexcept {
    OnEvent("OnObjectShot", [&](Runtime& runtime) {
        return Emit(runtime, Event::ObjectShot, Seen<EntityKind::Object>(runtime, object_id),
                    SeenPlayer(runtime, player_id), weapon);
    });
}

void OnObjectTouched(int32_t object_id, int32_t player_id) noexcept {
    OnEvent("OnObjectTouched", [&](Runtime& runtime) {
        return Emit(runtime, Event::ObjectTouch, Seen<EntityKind::Object>(runtime, object_id),
                    SeenPlayer(runtime, player_id));
    });
}

uint8_t OnPickupPickAttempt(int32_t pickup_id, int32_t player_id) noexcept {
    return OnEvent("OnPickupPickAttempt", [&](Runtime& runtime) {
        return Emit(runtime, Event::PickupPickAttempt, Seen<EntityKind::Pickup>(runtime, pickup_id),
                    SeenPlayer(runtime, player_id));
    });
}

void OnPickupPicked(int32_t pickup_id, int32_t player_id) noexcept {
    OnEvent("OnPickupPicked", [&](Runtime& runtime) {
        return Emit(runtime, Event::PickupPicked, Seen<EntityKind::Pickup>(runtime, pickup_id),
                    SeenPlayer(runtime, player_id));
    });
}

void OnPickupRespawn(int32_t pickup_id) noexcept {
    OnEvent("OnPickupRespawn", [&](Runtime& runtime) {
        return Emit(runtime, Event::PickupRespawn, Seen<EntityKind::Pickup>(runtime, pickup_id));
    });
}

void OnCheckpointEntered(int32_t checkpoint_id, int32_t player_id) noexcept {
    OnEvent("OnCheckpointEntered", [&](Runtime& runtime) {
        return Emit(runtime, Event::CheckpointEnter,
                    Seen<EntityKind::Checkpoint>(runtime, checkpoint_id),
                    SeenPlayer(runtime, player_id));
    });
}

void OnCheckpointExited(int32_t checkpoint_id, int32_t player_id) noexcept {
    OnEvent("OnCheckpointExited", [&](Runtime& runtime) {
        return Emit(runtime, Event::CheckpointExit,
                    Seen<EntityKind::Checkpoint>(runtime, checkpoint_id),
                    SeenPlayer(runtime, player_id));
    });
}

// onPlayerKeyDown / onPlayerKeyUp(player, bind). Binds raise no pool
// events; one seen here for the first time (another plugin's) is adopted.
void OnPlayerKeyBindDown(int32_t player_id, int32_t bind_id) noexcept {
    OnEvent("OnPlayerKeyBindDown", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerKeyDown, SeenPlayer(runtime, player_id),
                    Seen<EntityKind::Bind>(runtime, bind_id));
    });
}

void OnPlayerKeyBindUp(int32_t player_id, int32_t bind_id) noexcept {
    OnEvent("OnPlayerKeyBindUp", [&](Runtime& runtime) {
        return Emit(runtime, Event::PlayerKeyUp, SeenPlayer(runtime, player_id),
                    Seen<EntityKind::Bind>(runtime, bind_id));
    });
}

// onClientData(player, stream, size). The data is copied into a Stream of
// its own size, because the client controls size.
void OnClientScriptData(int32_t player_id, const uint8_t* data, size_t size) noexcept {
    OnEvent("OnClientScriptData", [&](Runtime& runtime) {
        if (!runtime.Events().HasHandlers(EventBus::Index(Event::ClientData))) {
            return false;
        }
        bindings::Stream stream;
        if (data != nullptr && size > 0) {
            stream.bytes.assign(data, data + size);
        }
        // One Lua object for every handler: each reads the same stream.
        sol::main_object object(runtime.state(), sol::in_place, std::move(stream));
        return Emit(runtime, Event::ClientData, SeenPlayer(runtime, player_id), object, size);
    });
}

uint8_t OnPluginCommand(uint32_t command, const char* message) noexcept {
    return OnEvent("OnPluginCommand", [&](Runtime& runtime) {
        return Emit(runtime, Event::PluginCommand, command, message);
    });
}

// onServerPerformanceReport(count, descriptions, times): two arrays of
// count entries.
void OnServerPerformanceReport(size_t count, const char** descriptions, uint64_t* times) noexcept {
    OnEvent("OnServerPerformanceReport", [&](Runtime& runtime) {
        if (!runtime.Events().HasHandlers(EventBus::Index(Event::ServerPerformanceReport))) {
            return false;
        }
        sol::main_table names(runtime.state(), sol::create);
        sol::main_table durations(runtime.state(), sol::create);
        for (size_t i = 0; descriptions != nullptr && times != nullptr && i < count; ++i) {
            names.raw_set(i + 1, descriptions[i] != nullptr ? descriptions[i] : "");
            durations.raw_set(i + 1, static_cast<lua_Integer>(times[i]));
        }
        return Emit(runtime, Event::ServerPerformanceReport, count, names, durations);
    });
}

// onEntityStreamingChange(player, entityType, entityId, deleted): new in
// plugin API 2.1; deleted is true when the entity streamed out.
void OnEntityStreamingChange(int32_t player_id, int32_t entity_id, vcmpEntityPool type,
                             uint8_t deleted) noexcept {
    OnEvent("OnEntityStreamingChange", [&](Runtime& runtime) {
        return Emit(runtime, Event::EntityStreamingChange, SeenPlayer(runtime, player_id),
                    static_cast<int32_t>(type), entity_id, deleted != 0);
    });
}

void SetPluginName(PluginInfo* info) noexcept {
    const std::size_t length = std::min(std::strlen(kPluginName), sizeof(info->name) - 1);
    std::memcpy(info->name, kPluginName, length);
    info->name[length] = '\0';
}

// Writes a callback only where the server's struct has room for it.
#define VCMP_LUA_SET_CALLBACK(calls, field, fn)                          \
    do {                                                                 \
        if (VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, field)) {         \
            (calls)->field = (fn);                                       \
        } else {                                                         \
            log::Warn("this server version has no " #field " callback"); \
        }                                                                \
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
            // Not PLUGIN_API_MINOR (1 in the 2.1 header): the 0.4 server
            // refuses a plugin whose minor version is above its own, and the
            // current one reports 2.0 although its structs already have the
            // 2.1 fields (docs/internals.md). Every field past 2.0 is used
            // only when structSize has room for it.
            info->apiMinorVersion = kApiMinorVersion;
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
        auto runtime =
            std::make_unique<Runtime>(std::move(config), plugin.api, ClockOf(plugin.options));
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
        VCMP_LUA_SET_CALLBACK(calls, OnIncomingConnection, &OnIncomingConnection);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerModuleList, &OnPlayerModuleList);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerRequestClass, &OnPlayerRequestClass);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerRequestSpawn, &OnPlayerRequestSpawn);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerSpawn, &OnPlayerSpawn);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerDeath, &OnPlayerDeath);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerUpdate, &OnPlayerUpdate);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerRequestEnterVehicle, &OnPlayerRequestEnterVehicle);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerEnterVehicle, &OnPlayerEnterVehicle);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerExitVehicle, &OnPlayerExitVehicle);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerNameChange, &OnPlayerNameChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerStateChange, &OnPlayerStateChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerActionChange, &OnPlayerActionChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerOnFireChange, &OnPlayerOnFireChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerCrouchChange, &OnPlayerCrouchChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerGameKeysChange, &OnPlayerGameKeysChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerBeginTyping, &OnPlayerBeginTyping);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerEndTyping, &OnPlayerEndTyping);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerAwayChange, &OnPlayerAwayChange);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerMessage, &OnPlayerMessage);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerCommand, &OnPlayerCommand);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerPrivateMessage, &OnPlayerPrivateMessage);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerSpectate, &OnPlayerSpectate);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerCrashReport, &OnPlayerCrashReport);
        VCMP_LUA_SET_CALLBACK(calls, OnClientScriptData, &OnClientScriptData);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerKeyBindDown, &OnPlayerKeyBindDown);
        VCMP_LUA_SET_CALLBACK(calls, OnPlayerKeyBindUp, &OnPlayerKeyBindUp);
        VCMP_LUA_SET_CALLBACK(calls, OnVehicleUpdate, &OnVehicleUpdate);
        VCMP_LUA_SET_CALLBACK(calls, OnVehicleExplode, &OnVehicleExplode);
        VCMP_LUA_SET_CALLBACK(calls, OnVehicleRespawn, &OnVehicleRespawn);
        VCMP_LUA_SET_CALLBACK(calls, OnObjectShot, &OnObjectShot);
        VCMP_LUA_SET_CALLBACK(calls, OnObjectTouched, &OnObjectTouched);
        VCMP_LUA_SET_CALLBACK(calls, OnPickupPickAttempt, &OnPickupPickAttempt);
        VCMP_LUA_SET_CALLBACK(calls, OnPickupPicked, &OnPickupPicked);
        VCMP_LUA_SET_CALLBACK(calls, OnPickupRespawn, &OnPickupRespawn);
        VCMP_LUA_SET_CALLBACK(calls, OnCheckpointEntered, &OnCheckpointEntered);
        VCMP_LUA_SET_CALLBACK(calls, OnCheckpointExited, &OnCheckpointExited);
        VCMP_LUA_SET_CALLBACK(calls, OnPluginCommand, &OnPluginCommand);
        VCMP_LUA_SET_CALLBACK(calls, OnServerPerformanceReport, &OnServerPerformanceReport);
        VCMP_LUA_SET_CALLBACK(calls, OnEntityStreamingChange, &OnEntityStreamingChange);
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
