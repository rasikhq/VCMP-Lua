#include "fake_server.hpp"

#include <doctest/doctest.h>
#include <lua.hpp>
#include <sol/sol.hpp>
#include <spdlog/sinks/ostream_sink.h>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>

#include "core/entity_pool.hpp"
#include "runtime/errors.hpp"
#include "runtime/log.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

FakeServer* g_current = nullptr;

// --- PluginFuncs ---------------------------------------------------------------

uint32_t GetServerVersion() { return 67710; }
uint32_t GetMaxPlayers() { return 100; }
uint64_t GetTime() { return static_cast<uint64_t>(FakeServer::Current().now_ms) * 1000; }
vcmpError GetLastError() { return vcmpErrorNone; }
vcmpError LogMessage(const char*, ...) { return vcmpErrorNone; }

uint8_t IsPlayerConnected(int32_t id) { return FakeServer::Current().Connected(id) ? 1 : 0; }

vcmpError GetPlayerName(int32_t id, char* buffer, size_t size) {
    if (!FakeServer::Current().Connected(id)) {
        return vcmpErrorNoSuchEntity;
    }
    std::snprintf(buffer, size, "player%d", id);
    return vcmpErrorNone;
}

vcmpError KickPlayer(int32_t id) { return FakeServer::Current().Kick(id); }

uint8_t CheckEntityExists(vcmpEntityPool pool, int32_t id) {
    return FakeServer::Current().EntityExists(pool, id) ? 1 : 0;
}

int32_t CreateVehicle(int32_t, int32_t, float, float, float, float, int32_t, int32_t) {
    return FakeServer::Current().CreateEntity(vcmpEntityPoolVehicle);
}
vcmpError DeleteVehicle(int32_t id) {
    return FakeServer::Current().DeleteEntity(vcmpEntityPoolVehicle, id);
}
int32_t CreateObject(int32_t, int32_t, float, float, float, int32_t) {
    return FakeServer::Current().CreateEntity(vcmpEntityPoolObject);
}
vcmpError DeleteObject(int32_t id) {
    return FakeServer::Current().DeleteEntity(vcmpEntityPoolObject, id);
}
int32_t CreatePickup(int32_t, int32_t, int32_t, float, float, float, int32_t, uint8_t) {
    return FakeServer::Current().CreateEntity(vcmpEntityPoolPickup);
}
vcmpError DeletePickup(int32_t id) {
    return FakeServer::Current().DeleteEntity(vcmpEntityPoolPickup, id);
}
int32_t CreateCheckPoint(int32_t, int32_t, uint8_t, float, float, float, int32_t, int32_t, int32_t,
                         int32_t, float) {
    return FakeServer::Current().CreateEntity(vcmpEntityPoolCheckPoint);
}
vcmpError DeleteCheckPoint(int32_t id) {
    return FakeServer::Current().DeleteEntity(vcmpEntityPoolCheckPoint, id);
}
int32_t CreateCoordBlip(int32_t, int32_t, float, float, float, int32_t, uint32_t, int32_t) {
    return FakeServer::Current().CreateEntity(vcmpEntityPoolBlip);
}
vcmpError DestroyCoordBlip(int32_t id) {
    return FakeServer::Current().DeleteEntity(vcmpEntityPoolBlip, id);
}

// --- Test hooks installed in every runtime -------------------------------------

// tostring() without metamethods: a __tostring could raise a Lua error
// through this C++ frame.
std::string Describe(lua_State* L, int index) {
    switch (lua_type(L, index)) {
        case LUA_TSTRING:
        case LUA_TNUMBER: {
            std::size_t length = 0;
            lua_pushvalue(L, index);
            const char* text = lua_tolstring(L, -1, &length);
            std::string result(text, length);
            lua_pop(L, 1);
            return result;
        }
        case LUA_TBOOLEAN:
            return lua_toboolean(L, index) ? "true" : "false";
        case LUA_TNIL:
            return "nil";
        default:
            return luaL_typename(L, index);
    }
}

template <EntityKind K>
EntityPool& Pool(lua_State* L) {
    return Runtime::Require(L).Entities().Get(K);
}

void InstallHooks(Runtime& runtime) {
    if (std::exchange(FakeServer::Current().fail_next_runtime, false)) {
        throw std::runtime_error("this runtime fails to start (test)");
    }
    sol::state_view lua(runtime.state());

    // record(...): appends the arguments, joined with spaces, to records.
    lua["record"] = [](sol::this_state L, sol::variadic_args args) {
        std::string line;
        for (int i = 0; i < static_cast<int>(args.size()); ++i) {
            if (i > 0) {
                line += ' ';
            }
            line += Describe(L, args.stack_index() + i);
        }
        FakeServer::Current().records.push_back(std::move(line));
    };
    lua["record_count"] = [] { return FakeServer::Current().records.size(); };

    // What a phase 3 Vehicle.create binding does: the server reports the new
    // vehicle inside CreateVehicle, so the adopt after it must be a no-op.
    lua["test_create_vehicle"] = [](sol::this_state L) {
        Runtime& rt = Runtime::Require(L);
        const int32_t id = VCMP_LUA_API(rt.api(), CreateVehicle)(130, 0, 0, 0, 0, 0, -1, -1);
        if (id < 0) {
            throw std::runtime_error("CreateVehicle failed");
        }
        EntityPool& pool = rt.Entities().Get(EntityKind::Vehicle);
        const VehicleRef vehicle = pool.Seen<EntityKind::Vehicle>(id);
        pool.MarkCreatedByUs(id);
        return vehicle;
    };

    lua["test_delete_vehicle"] = [](sol::this_state L,
                                    const EntityHandle<EntityKind::Vehicle>& vehicle) {
        EntityPool& pool = Pool<EntityKind::Vehicle>(L);
        pool.Require(vehicle.id, vehicle.generation);
        VCMP_LUA_API(Runtime::Require(L).api(), DeleteVehicle)(vehicle.id);
        pool.Release(vehicle.id);
    };

    lua["test_vehicle"] = [](sol::this_state L, int32_t id) {
        return Pool<EntityKind::Vehicle>(L).Ref<EntityKind::Vehicle>(id);
    };

    lua["test_player"] = [](sol::this_state L, int32_t id) {
        return Pool<EntityKind::Player>(L).Ref<EntityKind::Player>(id);
    };

    lua["test_kick"] = [](sol::this_state L, const EntityHandle<EntityKind::Player>& player) {
        Pool<EntityKind::Player>(L).Require(player.id, player.generation);
        VCMP_LUA_API(Runtime::Require(L).api(), KickPlayer)(player.id);
    };

    // The server shutting down while Lua runs (the runtime must defer it).
    lua["test_server_shutdown"] = [] { FakeServer::Current().calls.OnServerShutdown(); };
}

}  // namespace

FakeServer& FakeServer::Current() {
    if (g_current == nullptr) {
        throw std::logic_error("no FakeServer");
    }
    return *g_current;
}

FakeServer::FakeServer() {
    REQUIRE(g_current == nullptr);
    g_current = this;
    log::Init();
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log_);
    sink->set_pattern("[%l] %v");
    log::Logger()->sinks().push_back(sink);
    log_sink_ = sink.get();

    funcs.structSize = sizeof(funcs);
    calls.structSize = sizeof(calls);
    info.structSize = sizeof(info);
    funcs.GetServerVersion = &GetServerVersion;
    funcs.GetMaxPlayers = &GetMaxPlayers;
    funcs.GetTime = &GetTime;
    funcs.GetLastError = &GetLastError;
    funcs.LogMessage = &LogMessage;
    funcs.IsPlayerConnected = &IsPlayerConnected;
    funcs.GetPlayerName = &GetPlayerName;
    funcs.KickPlayer = &KickPlayer;
    funcs.CheckEntityExists = &CheckEntityExists;
    funcs.CreateVehicle = &vcmp_lua::test::CreateVehicle;
    funcs.DeleteVehicle = &vcmp_lua::test::DeleteVehicle;
    funcs.CreateObject = &CreateObject;
    funcs.DeleteObject = &DeleteObject;
    funcs.CreatePickup = &CreatePickup;
    funcs.DeletePickup = &DeletePickup;
    funcs.CreateCheckPoint = &CreateCheckPoint;
    funcs.DeleteCheckPoint = &DeleteCheckPoint;
    funcs.CreateCoordBlip = &CreateCoordBlip;
    funcs.DestroyCoordBlip = &DestroyCoordBlip;
}

FakeServer::~FakeServer() {
    if (running()) {
        Shutdown();
    }
    auto& sinks = log::Logger()->sinks();
    std::erase_if(sinks, [this](const spdlog::sink_ptr& sink) { return sink.get() == log_sink_; });
    log::SetLevel(spdlog::level::info);
    g_current = nullptr;
}

bool FakeServer::Load(Config config) {
    plugin::Options options;
    options.config = std::move(config);
    return LoadWith(std::move(options));
}

bool FakeServer::LoadConfigFile(const std::string& path) {
    plugin::Options options;
    options.config_path = path;
    return LoadWith(std::move(options));
}

bool FakeServer::LoadWith(plugin::Options options) {
    options.clock = [this] { return now_ms; };
    options.manage_libraries = false;
    options.on_runtime = &InstallHooks;
    loaded_ = plugin::Init(&funcs, &calls, &info, std::move(options)) == 1;
    return loaded_;
}

void FakeServer::Initialise() {
    REQUIRE(calls.OnServerInitialise != nullptr);
    calls.OnServerInitialise();
}

void FakeServer::Frame(std::int64_t advance_ms) {
    now_ms += advance_ms;
    calls.OnServerFrame(static_cast<float>(advance_ms) / 1000.0f);
}

void FakeServer::Shutdown() {
    calls.OnServerShutdown();
    shut_down_ = true;
    // What the real server does next (docs/internals.md).
    for (const vcmpEntityPool pool : {vcmpEntityPoolPickup, vcmpEntityPoolObject,
                                      vcmpEntityPoolCheckPoint, vcmpEntityPoolVehicle,
                                      vcmpEntityPoolBlip}) {
        const std::set<std::int32_t> ids = *PoolSet(pool);
        for (const std::int32_t id : ids) {
            DeleteEntity(pool, id);
        }
    }
    const std::set<std::int32_t> players = players_;
    for (const std::int32_t id : players) {
        Disconnect(id, vcmpDisconnectReasonTimeout);
    }
}

std::int32_t FakeServer::Connect(const std::string&) {
    std::int32_t id = 0;
    while (players_.contains(id)) {
        ++id;
    }
    players_.insert(id);
    calls.OnPlayerConnect(id);
    return id;
}

void FakeServer::Disconnect(std::int32_t id, vcmpDisconnectReason reason) {
    REQUIRE(players_.contains(id));
    calls.OnPlayerDisconnect(id, reason);  // still connected during the event
    players_.erase(id);
}

vcmpError FakeServer::Kick(std::int32_t id) {
    if (!players_.contains(id)) {
        return vcmpErrorNoSuchEntity;
    }
    Disconnect(id, vcmpDisconnectReasonKick);
    return vcmpErrorNone;
}

std::int32_t FakeServer::CreateVehicle() {
    return funcs.CreateVehicle(130, 0, 0, 0, 0, 0, -1, -1);
}

void FakeServer::DeleteVehicle(std::int32_t id) {
    REQUIRE(funcs.DeleteVehicle(id) == vcmpErrorNone);
}

std::set<std::int32_t>* FakeServer::PoolSet(vcmpEntityPool pool) {
    return const_cast<std::set<std::int32_t>*>(std::as_const(*this).PoolSet(pool));
}

const std::set<std::int32_t>* FakeServer::PoolSet(vcmpEntityPool pool) const {
    switch (pool) {
        case vcmpEntityPoolVehicle:
            return &vehicles_;
        case vcmpEntityPoolObject:
            return &objects_;
        case vcmpEntityPoolPickup:
            return &pickups_;
        case vcmpEntityPoolCheckPoint:
            return &checkpoints_;
        case vcmpEntityPoolBlip:
            return &blips_;
        default:
            return nullptr;
    }
}

std::int32_t FakeServer::CreateEntity(vcmpEntityPool pool) {
    std::set<std::int32_t>& ids = *PoolSet(pool);
    std::int32_t id = pool == vcmpEntityPoolVehicle ? 1 : 0;
    while (ids.contains(id)) {
        ++id;
    }
    ids.insert(id);
    if (calls.OnEntityPoolChange != nullptr) {
        calls.OnEntityPoolChange(pool, id, 0);
    }
    return id;
}

vcmpError FakeServer::DeleteEntity(vcmpEntityPool pool, std::int32_t id) {
    std::set<std::int32_t>& ids = *PoolSet(pool);
    if (!ids.contains(id)) {
        return vcmpErrorNoSuchEntity;
    }
    if (calls.OnEntityPoolChange != nullptr) {
        calls.OnEntityPoolChange(pool, id, 1);  // still exists during the event
    }
    ids.erase(id);
    return vcmpErrorNone;
}

bool FakeServer::EntityExists(vcmpEntityPool pool, std::int32_t id) const {
    const std::set<std::int32_t>* ids = PoolSet(pool);
    return ids != nullptr && ids->contains(id);
}

Runtime* FakeServer::runtime() const noexcept {
    return plugin::CurrentRuntime();
}

std::string FakeServer::Run(const std::string& code) {
    Runtime* rt = runtime();
    if (rt == nullptr || !rt->Usable()) {
        return "no usable runtime";
    }
    lua_State* L = rt->state();
    Invoker::Scope scope(rt->invoker());
    if (luaL_loadbufferx(L, code.data(), code.size(), "=test", "t") != LUA_OK) {
        std::string error = ErrorText(L, -1);
        lua_pop(L, 1);
        return error;
    }
    return ProtectedCall(L, 0, 0).value_or("");
}

std::string FakeServer::Eval(const std::string& expression) {
    Runtime* rt = runtime();
    if (rt == nullptr || !rt->Usable()) {
        return "no usable runtime";
    }
    lua_State* L = rt->state();
    Invoker::Scope scope(rt->invoker());
    const std::string code = "return tostring(" + expression + ")";
    if (luaL_loadbufferx(L, code.data(), code.size(), "=test", "t") != LUA_OK) {
        std::string error = ErrorText(L, -1);
        lua_pop(L, 1);
        return "error: " + error;
    }
    if (auto error = ProtectedCall(L, 0, 1)) {
        return "error: " + *error;
    }
    std::string value = ErrorText(L, -1);
    lua_pop(L, 1);
    return value;
}

}  // namespace vcmp_lua::test
