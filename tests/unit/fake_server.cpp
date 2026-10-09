#include "fake_server.hpp"

#include <doctest/doctest.h>
#include <fmt/format.h>
#include <lua.hpp>
#include <sol/sol.hpp>
#include <spdlog/sinks/ostream_sink.h>

#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "core/entity_pool.hpp"
#include "runtime/errors.hpp"
#include "runtime/log.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

FakeServer* g_current = nullptr;

// --- The recorder --------------------------------------------------------------

// One argument of a server call, as the recorder sees it.
struct Slot {
    enum class Kind {
        Int,
        UInt,
        Float,
        Double,
        Text,     // const char*: an input string
        MutText,  // char*: a buffer if a size follows, else an input string
        Size,     // size_t
        Out,      // pointer to a number: an out-parameter
        Other,    // any other pointer
    };
    Kind kind = Kind::Other;
    std::int64_t i = 0;
    std::uint64_t u = 0;
    double d = 0;
    const char* text = nullptr;
    char* buffer = nullptr;
    void (*write)(void*, double) = nullptr;  // for Out
    void* out = nullptr;
};

template <typename T>
void WriteOut(void* target, double value) {
    *static_cast<T*>(target) = static_cast<T>(value);
}

template <typename T>
Slot MakeSlot(T value) {
    Slot slot;
    if constexpr (std::is_same_v<T, std::size_t>) {
        slot.kind = Slot::Kind::Size;
        slot.u = value;
    } else if constexpr (std::is_enum_v<T>) {
        slot.kind = Slot::Kind::Int;
        slot.i = static_cast<std::int64_t>(value);
    } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        slot.kind = Slot::Kind::Int;
        slot.i = value;
    } else if constexpr (std::is_integral_v<T>) {
        slot.kind = Slot::Kind::UInt;
        slot.u = value;
    } else if constexpr (std::is_same_v<T, float>) {
        slot.kind = Slot::Kind::Float;
        slot.d = value;
    } else if constexpr (std::is_same_v<T, double>) {
        slot.kind = Slot::Kind::Double;
        slot.d = value;
    } else if constexpr (std::is_same_v<T, const char*>) {
        slot.kind = Slot::Kind::Text;
        slot.text = value;
    } else if constexpr (std::is_same_v<T, char*>) {
        slot.kind = Slot::Kind::MutText;
        slot.buffer = value;
    } else if constexpr (std::is_pointer_v<T> && std::is_arithmetic_v<std::remove_pointer_t<T>>) {
        slot.kind = Slot::Kind::Out;
        slot.write = &WriteOut<std::remove_pointer_t<T>>;
        slot.out = value;
    } else {
        static_assert(std::is_pointer_v<T>, "unexpected parameter type");
        slot.kind = Slot::Kind::Other;
    }
    return slot;
}

std::string Quote(const char* text) {
    return text == nullptr ? "null" : fmt::format("\"{}\"", text);
}

template <std::size_t N>
std::string Describe(const char* name, const std::array<Slot, N>& slots) {
    std::string line = fmt::format("{}(", name);
    for (std::size_t i = 0; i < N; ++i) {
        const Slot& slot = slots[i];
        if (i > 0) {
            line += ", ";
        }
        const bool buffer = slot.kind == Slot::Kind::MutText && i + 1 < N &&
                            slots[i + 1].kind == Slot::Kind::Size;
        switch (slot.kind) {
            case Slot::Kind::Int:
                line += fmt::format("{}", slot.i);
                break;
            case Slot::Kind::UInt:
            case Slot::Kind::Size:
                line += fmt::format("{}", slot.u);
                break;
            case Slot::Kind::Float:
                line += fmt::format("{}", static_cast<float>(slot.d));
                break;
            case Slot::Kind::Double:
                line += fmt::format("{}", slot.d);
                break;
            case Slot::Kind::Text:
                line += Quote(slot.text);
                break;
            case Slot::Kind::MutText:
                line += buffer ? "*" : Quote(slot.buffer);
                break;
            case Slot::Kind::Out:
            case Slot::Kind::Other:
                line += "*";
                break;
        }
    }
    return line + ")";
}

// Records the call, fills its out-parameters and buffers, sets the last
// error, and returns the configured value.
template <typename R, std::size_t N>
R Respond(const char* name, std::array<Slot, N>& slots) {
    FakeServer& server = FakeServer::Current();
    server.calls.push_back(Describe(name, slots));
    const auto error_it = server.errors.find(name);
    const vcmpError error = error_it != server.errors.end() ? error_it->second : vcmpErrorNone;
    server.last_error = error;

    const auto outs_it = server.outs.find(name);
    std::size_t next_out = 0;
    for (std::size_t i = 0; i < N; ++i) {
        Slot& slot = slots[i];
        if (slot.kind == Slot::Kind::Out && slot.out != nullptr) {
            double value = 0;
            if (outs_it != server.outs.end() && next_out < outs_it->second.size()) {
                value = outs_it->second[next_out];
            }
            ++next_out;
            slot.write(slot.out, value);
        } else if (slot.kind == Slot::Kind::MutText && i + 1 < N &&
                   slots[i + 1].kind == Slot::Kind::Size && slot.buffer != nullptr &&
                   slots[i + 1].u > 0) {
            const auto text_it = server.texts.find(name);
            const std::string text = text_it != server.texts.end() ? text_it->second : "";
            std::snprintf(slot.buffer, slots[i + 1].u, "%s", text.c_str());
        }
    }

    if constexpr (std::is_void_v<R>) {
        return;
    } else if constexpr (std::is_pointer_v<R>) {
        return nullptr;
    } else if constexpr (std::is_same_v<R, vcmpError>) {
        const auto it = server.returns.find(name);
        return it != server.returns.end() ? static_cast<vcmpError>(static_cast<int>(it->second))
                                          : error;
    } else {
        const auto it = server.returns.find(name);
        const double value = it != server.returns.end() ? it->second : 0;
        if constexpr (std::is_floating_point_v<R>) {
            return static_cast<R>(value);
        } else {
            return static_cast<R>(static_cast<std::int64_t>(value));
        }
    }
}

template <auto Field>
struct Stub;

// The recorder for PluginFuncs::*Field, or the override installed for it.
template <typename R, typename... A, R (*PluginFuncs::*Field)(A...)>
struct Stub<Field> {
    static inline const char* name = "";
    static inline R (*override)(A...) = nullptr;

    static R Call(A... args) {
        if (override != nullptr) {
            FakeServer& server = FakeServer::Current();
            std::array<Slot, sizeof...(A)> slots = {MakeSlot(args)...};
            server.calls.push_back(Describe(name, slots));
            server.last_error = vcmpErrorNone;
            return override(args...);
        }
        std::array<Slot, sizeof...(A)> slots = {MakeSlot(args)...};
        return Respond<R>(name, slots);
    }
};

template <auto Field>
void Install(PluginFuncs& funcs, const char* name) {
    Stub<Field>::name = name;
    Stub<Field>::override = nullptr;
    funcs.*Field = &Stub<Field>::Call;
}

// The function runs instead of the recorder; the call is still recorded.
template <auto Field, typename Fn>
void Override(Fn fn) {
    Stub<Field>::override = fn;
}

// The C-variadic functions: the bindings always pass "%s" and one string.
std::string Format(const char* format, va_list args) {
    std::array<char, 4096> buffer{};
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    return buffer.data();
}

vcmpError FakeLogMessage(const char* format, ...) {
    va_list args;
    va_start(args, format);
    const std::string text = Format(format, args);
    va_end(args);
    FakeServer::Current().calls.push_back(fmt::format("LogMessage({})", Quote(text.c_str())));
    return vcmpErrorNone;
}

vcmpError FakeSendPluginCommand(uint32_t id, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const std::string text = Format(format, args);
    va_end(args);
    FakeServer::Current().calls.push_back(
        fmt::format("SendPluginCommand({}, {})", id, Quote(text.c_str())));
    return vcmpErrorNone;
}

vcmpError FakeSendClientMessage(int32_t player, uint32_t colour, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const std::string text = Format(format, args);
    va_end(args);
    FakeServer& server = FakeServer::Current();
    server.calls.push_back(
        fmt::format("SendClientMessage({}, {}, {})", player, colour, Quote(text.c_str())));
    return server.Connected(player) ? vcmpErrorNone : vcmpErrorNoSuchEntity;
}

vcmpError FakeSendGameMessage(int32_t player, int32_t type, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const std::string text = Format(format, args);
    va_end(args);
    FakeServer& server = FakeServer::Current();
    server.calls.push_back(
        fmt::format("SendGameMessage({}, {}, {})", player, type, Quote(text.c_str())));
    return server.Connected(player) || player == -1 ? vcmpErrorNone : vcmpErrorNoSuchEntity;
}

// --- Stateful functions --------------------------------------------------------

void InstallStateful(PluginFuncs& funcs) {
    funcs.LogMessage = &FakeLogMessage;
    funcs.SendPluginCommand = &FakeSendPluginCommand;
    funcs.SendClientMessage = &FakeSendClientMessage;
    funcs.SendGameMessage = &FakeSendGameMessage;

    // GetLastError is not recorded: bindings call it after many getters.
    funcs.GetLastError = [] { return FakeServer::Current().last_error; };

    Override<&PluginFuncs::GetServerVersion>([]() -> uint32_t { return 67710; });
    Override<&PluginFuncs::GetMaxPlayers>([]() -> uint32_t { return 100; });
    Override<&PluginFuncs::GetTime>(
        []() { return static_cast<uint64_t>(FakeServer::Current().now_ms) * 1000; });

    Override<&PluginFuncs::IsPlayerConnected>(
        [](int32_t id) -> uint8_t { return FakeServer::Current().Connected(id) ? 1 : 0; });
    Override<&PluginFuncs::GetPlayerName>([](int32_t id, char* buffer, size_t size) {
        const auto name = FakeServer::Current().PlayerName(id);
        if (!name) {
            return vcmpErrorNoSuchEntity;
        }
        if (name->size() >= size) {
            return vcmpErrorBufferTooSmall;
        }
        std::snprintf(buffer, size, "%s", name->c_str());
        return vcmpErrorNone;
    });
    Override<&PluginFuncs::SetPlayerName>([](int32_t id, const char* name) {
        return FakeServer::Current().SetPlayerName(id, name);
    });
    Override<&PluginFuncs::KickPlayer>([](int32_t id) { return FakeServer::Current().Kick(id); });
    Override<&PluginFuncs::BanPlayer>([](int32_t id) { return FakeServer::Current().Kick(id); });

    // Records the bytes, in hex: SendClientScriptData(0, 01ff, 2).
    Override<&PluginFuncs::SendClientScriptData>([](int32_t id, const void* data, size_t size) {
        FakeServer& server = FakeServer::Current();
        std::string hex;
        for (size_t i = 0; i < size; ++i) {
            hex += fmt::format("{:02x}", static_cast<const uint8_t*>(data)[i]);
        }
        server.calls.back() = fmt::format("SendClientScriptData({}, {}, {})", id, hex, size);
        return server.Connected(id) ? vcmpErrorNone : vcmpErrorNoSuchEntity;
    });

    // Key binds: slots 0-255, shared by every plugin, no pool events.
    Override<&PluginFuncs::GetKeyBindUnusedSlot>([]() -> int32_t {
        const auto& binds = FakeServer::Current().key_binds;
        for (int32_t id = 0; id < 256; ++id) {
            if (!binds.contains(id)) {
                return id;
            }
        }
        return -1;
    });
    Override<&PluginFuncs::RegisterKeyBind>(
        [](int32_t id, uint8_t on_release, int32_t key1, int32_t key2, int32_t key3) {
            if (id < 0 || id >= 256) {
                return vcmpErrorArgumentOutOfBounds;
            }
            FakeServer::Current().key_binds[id] = {on_release != 0, {key1, key2, key3}};
            return vcmpErrorNone;
        });
    Override<&PluginFuncs::RemoveKeyBind>([](int32_t id) {
        return FakeServer::Current().key_binds.erase(id) == 1 ? vcmpErrorNone
                                                              : vcmpErrorNoSuchEntity;
    });
    Override<&PluginFuncs::RemoveAllKeyBinds>([] { FakeServer::Current().key_binds.clear(); });
    Override<&PluginFuncs::GetKeyBindData>([](int32_t id, uint8_t* on_release, int32_t* key1,
                                              int32_t* key2, int32_t* key3) {
        const auto& binds = FakeServer::Current().key_binds;
        const auto it = binds.find(id);
        if (it == binds.end()) {
            return vcmpErrorNoSuchEntity;
        }
        *on_release = it->second.on_release ? 1 : 0;
        *key1 = it->second.keys[0];
        *key2 = it->second.keys[1];
        *key3 = it->second.keys[2];
        return vcmpErrorNone;
    });

    Override<&PluginFuncs::CheckEntityExists>([](vcmpEntityPool pool, int32_t id) -> uint8_t {
        return FakeServer::Current().EntityExists(pool, id) ? 1 : 0;
    });
    Override<&PluginFuncs::CreateVehicle>(
        [](int32_t, int32_t, float, float, float, float, int32_t, int32_t) {
            return FakeServer::Current().CreateEntity(vcmpEntityPoolVehicle);
        });
    Override<&PluginFuncs::DeleteVehicle>(
        [](int32_t id) { return FakeServer::Current().DeleteEntity(vcmpEntityPoolVehicle, id); });
    Override<&PluginFuncs::CreateObject>([](int32_t, int32_t, float, float, float, int32_t) {
        return FakeServer::Current().CreateEntity(vcmpEntityPoolObject);
    });
    Override<&PluginFuncs::DeleteObject>(
        [](int32_t id) { return FakeServer::Current().DeleteEntity(vcmpEntityPoolObject, id); });
    Override<&PluginFuncs::CreatePickup>(
        [](int32_t, int32_t, int32_t, float, float, float, int32_t, uint8_t) {
            return FakeServer::Current().CreateEntity(vcmpEntityPoolPickup);
        });
    Override<&PluginFuncs::DeletePickup>(
        [](int32_t id) { return FakeServer::Current().DeleteEntity(vcmpEntityPoolPickup, id); });
    Override<&PluginFuncs::CreateCheckPoint>([](int32_t, int32_t, uint8_t, float, float, float,
                                                int32_t, int32_t, int32_t, int32_t, float) {
        return FakeServer::Current().CreateEntity(vcmpEntityPoolCheckPoint);
    });
    Override<&PluginFuncs::DeleteCheckPoint>([](int32_t id) {
        return FakeServer::Current().DeleteEntity(vcmpEntityPoolCheckPoint, id);
    });
    Override<&PluginFuncs::CreateCoordBlip>(
        [](int32_t, int32_t, float, float, float, int32_t, uint32_t, int32_t) {
            return FakeServer::Current().CreateEntity(vcmpEntityPoolBlip);
        });
    Override<&PluginFuncs::DestroyCoordBlip>(
        [](int32_t id) { return FakeServer::Current().DeleteEntity(vcmpEntityPoolBlip, id); });
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

// fake.*: the recorder and the fake world, for the Lua tests.
void InstallFakeTable(sol::state_view lua) {
    sol::table fake = lua.create_named_table("fake");

    // fake.calls(): every recorded server call since the last fake.clear().
    fake["calls"] = [] { return sol::as_table(FakeServer::Current().calls); };
    fake["clear"] = [] { FakeServer::Current().calls.clear(); };
    // fake.last(): the last recorded call, or nil.
    fake["last"] = []() -> std::optional<std::string> {
        const auto& calls = FakeServer::Current().calls;
        if (calls.empty()) {
            return std::nullopt;
        }
        return calls.back();
    };
    // fake.ret(name, value): what the function returns from now on.
    fake["ret"] = [](const std::string& name, double value) {
        FakeServer::Current().returns[name] = value;
    };
    // fake.out(name, {values}): the function's out-parameters, in order.
    fake["out"] = [](const std::string& name, std::vector<double> values) {
        FakeServer::Current().outs[name] = std::move(values);
    };
    // fake.text(name, text): what the function writes into its buffer.
    fake["text"] = [](const std::string& name, const std::string& text) {
        FakeServer::Current().texts[name] = text;
    };
    // fake.error(name, code): the error the function reports (0 clears it).
    fake["error"] = [](const std::string& name, int code) {
        FakeServer::Current().errors[name] = static_cast<vcmpError>(code);
    };
    // fake.remove(name): the server lacks this function.
    fake["remove"] = [](const std::string& name) {
        FakeServer& server = FakeServer::Current();
#define VCMP_LUA_FUNC(field)        \
    if (name == #field) {           \
        server.funcs.field = nullptr; \
        return;                     \
    }
#include "plugin_funcs.inc"
#undef VCMP_LUA_FUNC
        throw std::invalid_argument("unknown server function " + name);
    };

    // fake.connect([name]): a player joins; returns the handle.
    fake["connect"] = [](sol::this_state L, std::optional<std::string> name) {
        const int32_t id = FakeServer::Current().Connect(name.value_or(""));
        return Pool<EntityKind::Player>(L).Ref<EntityKind::Player>(id);
    };
    // fake.disconnect(id[, reason]): the player leaves.
    fake["disconnect"] = [](int32_t id, std::optional<int> reason) {
        FakeServer::Current().Disconnect(id,
                                         static_cast<vcmpDisconnectReason>(reason.value_or(1)));
    };
    // fake.bind(id, onRelease, k1, k2, k3): another plugin registers a key
    // bind; fake.unbind(id) removes it; fake.binds() counts them.
    fake["bind"] = [](int32_t id, bool on_release, int32_t k1, int32_t k2, int32_t k3) {
        FakeServer::Current().key_binds[id] = {on_release, {k1, k2, k3}};
    };
    fake["unbind"] = [](int32_t id) { FakeServer::Current().key_binds.erase(id); };
    fake["binds"] = [] { return FakeServer::Current().key_binds.size(); };
    // fake.exists(pool, id): whether the server has the entity.
    fake["exists"] = [](int pool, int32_t id) {
        return FakeServer::Current().EntityExists(static_cast<vcmpEntityPool>(pool), id);
    };
    // fake.create(pool): another plugin creates an entity; returns its id.
    fake["create"] = [](int pool) {
        return FakeServer::Current().CreateEntity(static_cast<vcmpEntityPool>(pool));
    };
    // fake.delete(pool, id): the server or another plugin deletes it.
    fake["delete"] = [](int pool, int32_t id) {
        return FakeServer::Current().DeleteEntity(static_cast<vcmpEntityPool>(pool), id) ==
               vcmpErrorNone;
    };
}

void InstallHooks(Runtime& runtime) {
    if (std::exchange(FakeServer::Current().fail_next_runtime, false)) {
        throw std::runtime_error("this runtime fails to start (test)");
    }
    sol::state_view lua(runtime.state());
    InstallFakeTable(lua);

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
    lua["test_server_shutdown"] = [] { FakeServer::Current().plugin.OnServerShutdown(); };
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
    plugin.structSize = sizeof(plugin);
    info.structSize = sizeof(info);
#define VCMP_LUA_FUNC(field) Install<&PluginFuncs::field>(funcs, #field);
#include "plugin_funcs.inc"
#undef VCMP_LUA_FUNC
    InstallStateful(funcs);
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
    loaded_ = plugin::Init(&funcs, &plugin, &info, std::move(options)) == 1;
    return loaded_;
}

void FakeServer::Initialise() {
    REQUIRE(plugin.OnServerInitialise != nullptr);
    plugin.OnServerInitialise();
}

void FakeServer::Frame(std::int64_t advance_ms) {
    now_ms += advance_ms;
    plugin.OnServerFrame(static_cast<float>(advance_ms) / 1000.0f);
}

void FakeServer::Shutdown() {
    plugin.OnServerShutdown();
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
    std::vector<std::int32_t> players;
    for (const auto& [id, name] : players_) {
        players.push_back(id);
    }
    for (const std::int32_t id : players) {
        Disconnect(id, vcmpDisconnectReasonTimeout);
    }
}

std::int32_t FakeServer::Connect(const std::string& name) {
    std::int32_t id = 0;
    while (players_.contains(id)) {
        ++id;
    }
    players_.emplace(id, name.empty() ? fmt::format("player{}", id) : name);
    if (plugin.OnPlayerConnect != nullptr) {
        plugin.OnPlayerConnect(id);
    }
    return id;
}

void FakeServer::Disconnect(std::int32_t id, vcmpDisconnectReason reason) {
    REQUIRE(players_.contains(id));
    if (plugin.OnPlayerDisconnect != nullptr) {
        plugin.OnPlayerDisconnect(id, reason);  // still connected during the event
    }
    players_.erase(id);
}

vcmpError FakeServer::Kick(std::int32_t id) {
    if (!players_.contains(id)) {
        return vcmpErrorNoSuchEntity;
    }
    Disconnect(id, vcmpDisconnectReasonKick);
    return vcmpErrorNone;
}

std::optional<std::string> FakeServer::PlayerName(std::int32_t id) const {
    const auto it = players_.find(id);
    if (it == players_.end()) {
        return std::nullopt;
    }
    return it->second;
}

vcmpError FakeServer::SetPlayerName(std::int32_t id, const std::string& name) {
    const auto it = players_.find(id);
    if (it == players_.end()) {
        return vcmpErrorNoSuchEntity;
    }
    if (name.empty() || name.size() > 23) {
        return vcmpErrorInvalidName;
    }
    it->second = name;
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
    if (plugin.OnEntityPoolChange != nullptr) {
        plugin.OnEntityPoolChange(pool, id, 0);
    }
    return id;
}

vcmpError FakeServer::DeleteEntity(vcmpEntityPool pool, std::int32_t id) {
    std::set<std::int32_t>& ids = *PoolSet(pool);
    if (!ids.contains(id)) {
        return vcmpErrorNoSuchEntity;
    }
    if (plugin.OnEntityPoolChange != nullptr) {
        plugin.OnEntityPoolChange(pool, id, 1);  // still exists during the event
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

std::string FakeServer::Run(const std::string& code, const std::string& chunkname) {
    Runtime* rt = runtime();
    if (rt == nullptr || !rt->Usable()) {
        return "no usable runtime";
    }
    lua_State* L = rt->state();
    Invoker::Scope scope(rt->invoker());
    if (luaL_loadbufferx(L, code.data(), code.size(), chunkname.c_str(), "t") != LUA_OK) {
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
