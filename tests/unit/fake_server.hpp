#pragma once

// FakeServer: an in-memory VC:MP server for the unit tests. It hands the
// plugin a PluginFuncs table, drives its callbacks like the real server, and
// reproduces the behaviour measured in docs/internals.md:
//
// - Create*/Delete* report OnEntityPoolChange synchronously, before they
//   return; ids are reused at once, the first vehicle id is 1.
// - KickPlayer and BanPlayer report OnPlayerDisconnect synchronously.
// - Shutdown() sends OnServerShutdown, then the pool "deleted" events, then
//   the disconnects of the players still online (reason 0).
//
// Every other server function is a recorder: it appends "Name(arg, ...)" to
// calls (strings quoted, out-parameters and buffers as *), writes the values
// a test configured into its out-parameters and buffers, and returns the
// configured value (0, vcmpErrorNone or "" by default). A field can be
// cleared to test "not supported by this server version".

#include <vcmp.h>

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "plugin/plugin.hpp"

struct lua_State;

namespace vcmp_lua {
class Runtime;
}

namespace vcmp_lua::test {

class FakeServer {
public:
    // The clock starts just below 2^32 microseconds, where 2.x's timers stopped.
    static constexpr std::int64_t kStartMs = 4'294'000;

    FakeServer();
    ~FakeServer();  // shuts the plugin down if it is still running

    FakeServer(const FakeServer&) = delete;
    FakeServer& operator=(const FakeServer&) = delete;

    // VcmpPluginInit with config (no file) and this server's clock. The test
    // hooks (the fake table, test_* functions and record(); see
    // fake_server.cpp) are installed in every runtime.
    bool Load(Config config = {});
    // The same, reading the config from a luaconfig.lua file (also on reload).
    bool LoadConfigFile(const std::string& path);

    void Initialise();
    void Frame(std::int64_t advance_ms = 16);
    void Shutdown();
    [[nodiscard]] bool running() const noexcept { return loaded_ && !shut_down_; }

    // Players. Connect returns the new id (the lowest free one).
    std::int32_t Connect(const std::string& name = "");
    void Disconnect(std::int32_t id, vcmpDisconnectReason reason = vcmpDisconnectReasonQuit);
    [[nodiscard]] bool Connected(std::int32_t id) const { return players_.contains(id); }

    // Entities, as another plugin would create them (through PluginFuncs).
    std::int32_t CreateVehicle();
    void DeleteVehicle(std::int32_t id);
    [[nodiscard]] bool VehicleExists(std::int32_t id) const { return vehicles_.contains(id); }

    // The plugin's current runtime (changes on reload), or null.
    [[nodiscard]] Runtime* runtime() const noexcept;

    // Runs Lua source in the current runtime, as a call into Lua. Returns
    // the error, or an empty string. chunkname as for luaL_loadbuffer.
    std::string Run(const std::string& code, const std::string& chunkname = "=test");

    // Evaluates a Lua expression and returns tostring() of its value.
    std::string Eval(const std::string& expression);

    // What scripts passed to record(...), one string per call (arguments
    // joined with spaces). Survives reloads.
    std::vector<std::string> records;

    // The next runtime the plugin creates fails to start (a reload test).
    bool fail_next_runtime = false;

    // Everything the plugin logged while this server existed.
    [[nodiscard]] std::string LogText() const { return log_.str(); }

    // The recorder.

    // Server calls in order, e.g. SetPlayerHealth(0, 50) or
    // GetPlayerPosition(0, *, *, *). GetLastError is not recorded.
    std::vector<std::string> calls;
    // Return value of a function, by name (default 0 / vcmpErrorNone).
    std::map<std::string, double> returns;
    // Values for a function's numeric out-parameters, in order (default 0).
    std::map<std::string, std::vector<double>> outs;
    // Text for a function's char* buffer (default "").
    std::map<std::string, std::string> texts;
    // Error a function reports: returned by functions that return a
    // vcmpError, and from GetLastError() after the call.
    std::map<std::string, vcmpError> errors;
    vcmpError last_error = vcmpErrorNone;

    PluginFuncs funcs{};
    PluginCallbacks plugin{};  // the callbacks the plugin installed
    PluginInfo info{};
    std::int64_t now_ms = kStartMs;

    // Called from the static PluginFuncs entries.
    static FakeServer& Current();
    std::int32_t CreateEntity(vcmpEntityPool pool);
    vcmpError DeleteEntity(vcmpEntityPool pool, std::int32_t id);
    [[nodiscard]] bool EntityExists(vcmpEntityPool pool, std::int32_t id) const;
    vcmpError Kick(std::int32_t id);
    std::optional<std::string> PlayerName(std::int32_t id) const;
    vcmpError SetPlayerName(std::int32_t id, const std::string& name);

    struct KeyBind {
        bool on_release = false;
        std::int32_t keys[3] = {-1, -1, -1};
    };
    std::map<std::int32_t, KeyBind> key_binds;

private:
    bool LoadWith(plugin::Options options);
    std::set<std::int32_t>* PoolSet(vcmpEntityPool pool);
    const std::set<std::int32_t>* PoolSet(vcmpEntityPool pool) const;

    std::map<std::int32_t, std::string> players_;
    std::set<std::int32_t> vehicles_;
    std::set<std::int32_t> objects_;
    std::set<std::int32_t> pickups_;
    std::set<std::int32_t> checkpoints_;
    std::set<std::int32_t> blips_;
    bool loaded_ = false;
    bool shut_down_ = false;
    std::ostringstream log_;
    void* log_sink_ = nullptr;
};

}  // namespace vcmp_lua::test
