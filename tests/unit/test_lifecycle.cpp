// Runtime lifecycle (plan B3.1): server events, shutdown with live
// finalizers, deferred shutdown and reload, Lua panics.
#include <doctest/doctest.h>

#include <lua.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "fake_server.hpp"
#include "runtime/errors.hpp"
#include "runtime/log.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

// A script file for the tests that need one, deleted afterwards.
class ScriptFile {
public:
    explicit ScriptFile(const std::string& source) {
        static int counter = 0;
        path_ = std::filesystem::temp_directory_path() /
                ("vcmp_lua_test_" + std::to_string(++counter) + ".lua");
        std::ofstream(path_) << source;
    }
    ~ScriptFile() { std::filesystem::remove(path_); }
    ScriptFile(const ScriptFile&) = delete;
    ScriptFile& operator=(const ScriptFile&) = delete;

    [[nodiscard]] std::string path() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

Config WithScript(const ScriptFile& script) {
    Config config;
    config.scripts = {script.path()};
    return config;
}

}  // namespace

TEST_CASE("scripts load in OnServerInitialise, then onServerInit and onServerFrame run") {
    ScriptFile script(R"(
        record("script runs", test_player ~= nil)
        Event.bind("onServerInit", function() record("init") end)
        Event.bind("onServerFrame", function(elapsed) record("frame", elapsed) end)
    )");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    CHECK(server.records.empty());  // nothing runs in VcmpPluginInit
    server.Initialise();
    server.Frame(250);
    const std::vector<std::string> expected = {"script runs true", "init", "frame 0.25"};
    CHECK(server.records == expected);
}

TEST_CASE("shutdown: onServerShutdown, disconnects of online players, then finalizers") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    const int32_t id = server.Connect();
    REQUIRE(server.Run(R"(
        player = test_player(0)
        Event.bind("onServerShutdown", function() record("shutdown event") end)
        Event.bind("onPlayerDisconnect", function(p, reason)
            record("disconnect", p.id, reason)
        end)
        local function try(name, fn)
            local ok, err = pcall(fn)
            record(name, ok, err)
        end
        finalizer = setmetatable({}, { __gc = function()
            record("finalizer runs")
            try("Timer.create", function() Timer.create(print, 100, 1) end)
            try("Event.bind", function() Event.bind("onServerInit", print) end)
            try("Server.reload", function() Server.reload() end)
            try("player.id", function() return player.id end)
            record("tostring", tostring(player))
        end })
        Timer.create(function() record("timer must not run") end, 0, -1)
    )") == "");
    server.Shutdown();
    CHECK(server.runtime() == nullptr);
    const std::vector<std::string> expected = {
        "shutdown event",
        "disconnect " + std::to_string(id) + " 0",
        "finalizer runs",
        "Timer.create false runtime shutting down",
        "Event.bind false runtime shutting down",
        "Server.reload false runtime shutting down",
        "player.id false runtime shutting down",
        "tostring Player(0, no longer exists)",
    };
    CHECK(server.records == expected);
    // The server's own disconnect arrived after the runtime closed: ignored.
    CHECK_FALSE(server.Connected(id));

    // More events after shutdown are no-ops.
    server.Frame(100);
    server.calls.OnServerShutdown();
    server.calls.OnPlayerConnect(3);
    server.calls.OnEntityPoolChange(vcmpEntityPoolVehicle, 1, 0);
    CHECK(server.records == expected);
}

TEST_CASE("a shutdown that arrives during a Lua call waits until the call returns") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerConnect", function()
            test_server_shutdown()
            -- Still inside the callback: the runtime is intact.
            record("still running", (pcall(Timer.create, print, 100, 1)))
        end)
        Event.bind("onServerShutdown", function() record("shutdown event") end)
    )") == "");
    Runtime* before = server.runtime();
    server.Connect();
    CHECK(server.runtime() == nullptr);
    CHECK(before != nullptr);
    const std::vector<std::string> expected = {"still running true", "shutdown event"};
    CHECK(server.records == expected);
}

TEST_CASE("a reload requested in a handler runs at the end of the frame") {
    ScriptFile script(R"(
        record("load")
        Event.bind("onServerInit", function() record("init") end)
        if record_count() == 1 then mine = test_create_vehicle() end  -- first load only
        function request_reload()
            Server.reload()
            record("after reload call", Timer.create(print, 1000, 1) ~= nil)
        end
        Event.bind("onPlayerConnect", function() request_reload() end)
    )");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    const int32_t theirs = server.CreateVehicle();  // another plugin's
    server.Initialise();
    const std::string mine = server.Eval("mine.id");
    REQUIRE(mine != std::to_string(theirs));
    REQUIRE(server.Run("marker = 'old state'") == "");

    const int32_t player = server.Connect();  // the handler requests the reload
    CHECK(server.Eval("marker") == "old state");  // not yet: the frame has not ended
    std::vector<std::string> expected = {"load", "init", "after reload call true"};
    CHECK(server.records == expected);

    server.Frame();
    REQUIRE(server.runtime() != nullptr);
    CHECK(server.Eval("marker") == "nil");  // a new Lua state
    expected.insert(expected.end(), {"load", "init"});
    CHECK(server.records == expected);

    // The old runtime's vehicle is gone.
    CHECK_FALSE(server.VehicleExists(std::stoi(mine)));
    // Another plugin's vehicle and the connected player were adopted again.
    CHECK(server.VehicleExists(theirs));
    CHECK(server.Eval("test_vehicle(" + std::to_string(theirs) + ") ~= nil") == "true");
    CHECK(server.Eval("test_player(" + std::to_string(player) + ") ~= nil") == "true");
}

TEST_CASE("a reload requested in a timer runs at the end of that frame") {
    ScriptFile script(R"(
        record("load")
        Timer.create(function() Server.reload() record("timer continues") end, 100, 1)
    )");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    server.Initialise();
    server.Frame(100);
    const std::vector<std::string> expected = {"load", "timer continues", "load"};
    CHECK(server.records == expected);
}

TEST_CASE("a handler that kicks another player during the shutdown disconnects") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    const int32_t first = server.Connect();
    const int32_t second = server.Connect();
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerDisconnect", function(player, reason)
            record("disconnect", player.id, reason)
            local other = test_player(1 - player.id)
            if other and not kicked then
                kicked = true
                test_kick(other)
            end
        end)
    )") == "");
    server.Shutdown();
    // Each player once: the kicked one with the kick reason, nobody as nil.
    const std::vector<std::string> expected = {
        "disconnect " + std::to_string(first) + " 0",
        "disconnect " + std::to_string(second) + " 2",
    };
    CHECK(server.records == expected);
}

TEST_CASE("a reload whose new runtime cannot start keeps the old scripts running") {
    ScriptFile script("record('load')");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    server.Initialise();
    REQUIRE(server.Run("marker = 'old state'") == "");
    server.fail_next_runtime = true;
    REQUIRE(server.Run("Server.reload()") == "");
    server.Frame();
    REQUIRE(server.runtime() != nullptr);
    CHECK(server.Eval("marker") == "old state");
    CHECK_FALSE(server.runtime()->ReloadRequested());
    CHECK(server.LogText().find("the scripts keep running unchanged") != std::string::npos);
    const std::vector<std::string> expected = {"load"};
    CHECK(server.records == expected);
}

TEST_CASE("a shutdown that arrives while a reload loads the scripts runs in the same frame") {
    ScriptFile script(R"(
        record("load")
        Event.bind("onServerShutdown", function() record("shutdown event") end)
        if record_count() > 1 then test_server_shutdown() end
    )");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    server.Initialise();
    REQUIRE(server.Run("Server.reload()") == "");
    server.Frame();
    CHECK(server.runtime() == nullptr);
    const std::vector<std::string> expected = {"load", "load", "shutdown event"};
    CHECK(server.records == expected);
}

TEST_CASE("a script that fails during a reload does not stop the new runtime") {
    ScriptFile script("record('load') if record_count() > 1 then error('broken on reload') end");
    FakeServer server;
    REQUIRE(server.Load(WithScript(script)));
    server.Initialise();
    REQUIRE(server.Run("Server.reload()") == "");
    server.Frame();
    REQUIRE(server.runtime() != nullptr);
    CHECK(server.runtime()->Usable());
    const std::vector<std::string> expected = {"load", "load"};
    CHECK(server.records == expected);
    CHECK(server.LogText().find("broken on reload") != std::string::npos);
}

TEST_CASE("a reload with a broken luaconfig.lua keeps the old scripts running") {
    ScriptFile script("record('load')");
    ScriptFile config("return { scripts = { [[" + script.path() + "]] } }");
    FakeServer server;
    REQUIRE(server.LoadConfigFile(config.path()));
    server.Initialise();
    Runtime* old = server.runtime();

    std::ofstream(config.path()) << "return 42";
    REQUIRE(server.Run("Server.reload()") == "");
    server.Frame();
    CHECK(server.runtime() == old);
    CHECK(old->Usable());
    CHECK_FALSE(old->ReloadRequested());
    CHECK(server.LogText().find("must return a table (got number)") != std::string::npos);
    CHECK(server.LogText().find("the scripts keep running unchanged") != std::string::npos);
    const std::vector<std::string> expected = {"load"};
    CHECK(server.records == expected);
}

TEST_CASE("a Lua panic marks the runtime dead and every callback becomes a no-op") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(R"(
        Timer.create(function() record("timer") end, 0, -1)
        Event.bind("onPlayerConnect", function() record("connect") end)
    )") == "");
    Runtime* runtime = server.runtime();
    lua_State* L = runtime->state();
    lua_pushliteral(L, "deliberate error outside a protected call");
    CHECK_THROWS_AS(lua_error(L), LuaPanic);
    CHECK(runtime->Dead());
    CHECK_FALSE(runtime->Usable());

    server.Frame(100);
    const int32_t id = server.Connect();
    server.CreateVehicle();
    server.calls.OnServerInitialise();
    server.Disconnect(id);
    CHECK(server.records.empty());
    server.Shutdown();  // the dead state is leaked, not closed
    CHECK(server.runtime() == nullptr);
}

TEST_CASE("log.file sends the log to a file too") {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "vcmp_lua_test_logs";
    std::filesystem::remove_all(dir);
    const std::string file = (dir / "plugin.log").string();
    ScriptFile script("print('hello from the script')");
    ScriptFile config("return { scripts = { [[" + script.path() + "]] }, log = { file = [[" + file +
                      "]] } }");
    {
        FakeServer server;
        REQUIRE(server.LoadConfigFile(config.path()));
        server.Initialise();
    }
    std::ifstream log(file);
    const std::string text((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
    CHECK(text.find("hello from the script") != std::string::npos);
    log.close();
    log::Configure(LogConfig{});  // closes the file
    std::filesystem::remove_all(dir);
}

TEST_CASE("init refuses a second plugin instance and a server without OnServerFrame") {
    {
        FakeServer server;
        REQUIRE(server.Load());
        PluginCallbacks calls{};
        calls.structSize = sizeof(calls);
        PluginInfo info{};
        info.structSize = sizeof(info);
        plugin::Options options;
        options.manage_libraries = false;
        CHECK(plugin::Init(&server.funcs, &calls, &info, std::move(options)) == 0);
        CHECK(server.LogText().find("already initialised") != std::string::npos);
    }
    {
        FakeServer server;
        server.calls.structSize = static_cast<uint32_t>(offsetof(PluginCallbacks, OnServerFrame));
        CHECK_FALSE(server.Load());
        CHECK(server.runtime() == nullptr);
    }
}

}  // namespace vcmp_lua::test
