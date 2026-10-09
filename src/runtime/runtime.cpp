#include "runtime/runtime.hpp"

#include <lua.hpp>

#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "bindings/bindings.hpp"
#include "runtime/errors.hpp"
#include "runtime/log.hpp"
#include "runtime/preload.hpp"

namespace vcmp_lua {
namespace {

// The lua_CFunctions below run inside lua_pcall. A Lua error unwinds them
// with longjmp, so none of them may own a C++ object with a destructor.

// print(...): one line in the plugin log.
int Print(lua_State* L) {
    const int count = lua_gettop(L);
    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);
    for (int i = 1; i <= count; ++i) {
        if (i > 1) {
            luaL_addchar(&buffer, '\t');
        }
        luaL_tolstring(L, i, nullptr);
        luaL_addvalue(&buffer);
    }
    luaL_pushresult(&buffer);
    std::size_t length = 0;
    const char* text = lua_tolstring(L, -1, &length);
    log::Info("{}", std::string_view(text, length));
    return 0;
}

// Arg 1: light userdata, const Config*. Returns the prelude's table.
int SetupState(lua_State* L) {
    const auto* config = static_cast<const Config*>(lua_touserdata(L, 1));
    luaL_openlibs(L);
    RegisterBuiltins(L);

    lua_getglobal(L, LUA_LOADLIBNAME);
    lua_pushlstring(L, config->package_path.data(), config->package_path.size());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    lua_pushcfunction(L, &Print);
    lua_setglobal(L, "print");

    RunPrelude(L);
    return 1;
}

Runtime::Remains* g_remains = nullptr;

// Arg 1: light userdata, NUL-terminated path. Returns the loaded chunk.
// Text mode only: crafted bytecode can crash Lua 5.4 (plan B6).
int LoadScriptFile(lua_State* L) {
    const auto* path = static_cast<const char*>(lua_touserdata(L, 1));
    if (luaL_loadfilex(L, path, "t") != LUA_OK) {
        return lua_error(L);
    }
    return 1;
}

}  // namespace

// What a runtime that died in a Lua panic leaves behind. It is leaked on
// purpose, but stays reachable from g_remains, so leak checkers do not report
// it. Allocated up front: the dead path must not depend on an allocation.
struct Runtime::Remains {
    std::optional<sol::state> lua;
    std::unique_ptr<Invoker> invoker;
    std::unique_ptr<EntityPools> entities;
    std::unique_ptr<EventBus> events;
    std::unique_ptr<Scheduler> timers;
    std::unique_ptr<Http> http;
    std::unique_ptr<FramePump> pump;
    Remains* next = nullptr;
};

Runtime::Runtime(Config config, ServerApi api, Clock clock)
    : config_(std::move(config)), api_(api), remains_(std::make_unique<Remains>()) {
    lua_.emplace(&Panic);
    lua_State* L = lua_->lua_state();
    *static_cast<Runtime**>(lua_getextraspace(L)) = this;
    try {
        invoker_ = std::make_unique<Invoker>(L);
        entities_ = std::make_unique<EntityPools>();
        events_ = std::make_unique<EventBus>(*invoker_);
        timers_ = std::make_unique<Scheduler>(*invoker_, std::move(clock));
        http_ = std::make_unique<Http>(*invoker_, config_.http);
        pump_ = std::make_unique<FramePump>();

        lua_pushcfunction(L, &SetupState);
        lua_pushlightuserdata(L, &config_);
        if (auto error = ProtectedCall(L, 1, 1)) {
            throw std::runtime_error("cannot set up the Lua state: " + *error);
        }
        sol::main_table prelude(L, -1);
        lua_pop(L, 1);
        bindings::Register(*lua_);

        // Once per frame, in this order: due timers, finished HTTP
        // requests, one Copas step.
        pump_->Add([this](lua_State* thread) { timers_->Tick(thread); });
        pump_->Add([this](lua_State* thread) { http_->Pump(thread); });
        pump_->Add([this, copas_step = prelude.raw_get<sol::main_protected_function>(
                              "copas_step")](lua_State* thread) {
            invoker_->Call(thread, copas_step, "copas.step", [](lua_State*) { return 0; });
        });
    } catch (...) {
        Shutdown(ShutdownReason::Server);
        throw;
    }
}

Runtime::~Runtime() {
    Shutdown(ShutdownReason::Server);
}

bool Runtime::Usable() const noexcept {
    return lua_.has_value() && !closing_ && !dead_;
}

bool Runtime::InLuaCall() const noexcept {
    return invoker_ != nullptr && invoker_->depth() > 0;
}

void Runtime::MarkDead() noexcept {
    dead_ = true;
    if (invoker_ != nullptr) {
        invoker_->MarkDead();
    }
}

Runtime* Runtime::FromState(lua_State* L) noexcept {
    return *static_cast<Runtime**>(lua_getextraspace(L));
}

Runtime& Runtime::Require(lua_State* L) {
    Runtime* runtime = FromState(L);
    if (runtime == nullptr || !runtime->Usable()) {
        throw std::runtime_error("runtime shutting down");
    }
    return *runtime;
}

void Runtime::LoadScripts() {
    if (!Usable()) {
        return;
    }
    entities_->Enumerate(api_);
    for (const std::string& script : config_.scripts) {
        if (!Usable()) {
            return;
        }
        lua_State* L = lua_->lua_state();
        Invoker::Scope scope(*invoker_);
        log::Info("Loading {}", script);

        lua_pushcfunction(L, &LoadScriptFile);
        lua_pushlightuserdata(L, const_cast<char*>(script.c_str()));
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            log::Error("{}", ErrorText(L, -1));
            lua_pop(L, 1);
            continue;
        }
        if (auto error = ProtectedCall(L, 0, 0)) {
            log::Error("{}", *error);
        }
    }
    if (Usable()) {
        events_->Emit(lua_->lua_state(), Event::ServerInit);
    }
}

void Runtime::Frame(float elapsed_seconds) {
    if (!Usable()) {
        return;
    }
    lua_State* L = lua_->lua_state();
    pump_->Run(L);
    if (Usable()) {
        events_->Emit(L, Event::ServerFrame, elapsed_seconds);
    }
}

void Runtime::NotifyServerShutdown() {
    if (!Usable()) {
        return;
    }
    lua_State* L = lua_->lua_state();
    events_->Emit(L, Event::ServerShutdown);
    EntityPool& players = entities_->Players();
    for (const std::int32_t id : players.AliveIds()) {
        if (!Usable()) {
            return;
        }
        if (!players.Alive(id)) {
            continue;  // a handler kicked this player meanwhile
        }
        events_->Emit(L, Event::PlayerDisconnect, players.Ref<EntityKind::Player>(id),
                      vcmpDisconnectReasonTimeout);
        players.Release(id);
    }
}

void Runtime::Shutdown(ShutdownReason reason) noexcept {
    if (!lua_.has_value()) {
        return;
    }
    if (InLuaCall()) {
        // A caller bug: Lua is still running. The plugin defers shutdowns
        // and reloads until no call is active, so this is never reached.
        log::Error("Runtime::Shutdown during a call into Lua; ignored");
        return;
    }
    // 1. From here on bindings raise "runtime shutting down" and no new
    //    entity handle is made.
    closing_ = true;

    // After a panic the Lua state is inconsistent: not even releasing a
    // reference is safe. The state and everything that refers into it are
    // leaked instead.
    if (dead_) {
        Remains* remains = remains_.release();
        remains->lua = std::move(lua_);
        remains->invoker = std::move(invoker_);
        remains->entities = std::move(entities_);
        remains->events = std::move(events_);
        remains->timers = std::move(timers_);
        remains->http = std::move(http_);
        remains->pump = std::move(pump_);
        remains->next = std::exchange(g_remains, remains);
        lua_.reset();  // moved from: closes nothing
        return;
    }

    // A reload removes what this runtime created. The server reports those
    // deletions while closing_ is set, so they only update the pools.
    if (entities_ != nullptr) {
        entities_->Close();
        if (reason == ShutdownReason::Reload) {
            try {
                entities_->DeleteCreated(api_);
            } catch (...) {
                log::Error("cannot delete the entities of the old runtime");
            }
        }
    }

    // 2. Release every Lua reference C++ holds.
    if (pump_ != nullptr) {
        pump_->Clear();
    }
    if (timers_ != nullptr) {
        timers_->Clear();
    }
    if (http_ != nullptr) {
        http_->Clear();  // pending requests are dropped without a callback
    }
    if (events_ != nullptr) {
        events_->Clear();
    }
    if (entities_ != nullptr) {
        entities_->ReleaseRefs();
    }

    // 3. Close the Lua state: __gc and __close run while the subsystems are
    //    still alive (and empty).
    lua_.reset();

    // 4. Destroy the subsystems.
    pump_.reset();
    http_.reset();
    timers_.reset();
    events_.reset();
    entities_.reset();
    invoker_.reset();
}

}  // namespace vcmp_lua
