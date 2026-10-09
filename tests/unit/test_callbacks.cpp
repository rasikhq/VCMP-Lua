// The server callbacks of plugin/callbacks.cpp: the events they dispatch,
// their arguments (v1's), and what cancellable ones return to the server.
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "fake_server.hpp"

namespace vcmp_lua::test {
namespace {

void Start(FakeServer& server) {
    REQUIRE(server.Load());
    server.Initialise();
}

// Binds a handler that records the event name and its arguments.
void Record(FakeServer& server, const std::string& event) {
    REQUIRE(server.Run("Event.bind('" + event + "', function(...) local t = {'" + event +
                       "'} for i = 1, select('#', ...) do local v = select(i, ...) "
                       "t[#t + 1] = type(v) == 'userdata' and tostring(v) or "
                       "type(v) == 'table' and ('{' .. table.concat(v, ',') .. '}') or "
                       "(type(v) .. ':' .. tostring(v)) end record(table.concat(t, ' ')) end)") ==
            "");
}

}  // namespace

TEST_CASE("player events pass v1's arguments") {
    FakeServer server;
    Start(server);
    for (const char* event : {"onPlayerConnection",
                              "onPlayerModuleList",
                              "onPlayerRequestClass",
                              "onPlayerRequestSpawn",
                              "onPlayerSpawn",
                              "onPlayerUpdate",
                              "onPlayerRequestEnterVehicle",
                              "onPlayerEnterVehicle",
                              "onPlayerExitVehicle",
                              "onPlayerNameChange",
                              "onPlayerStateChange",
                              "onPlayerActionChange",
                              "onPlayerFireChange",
                              "onPlayerCrouchChange",
                              "onPlayerGameKeysChange",
                              "onPlayerBeginTyping",
                              "onPlayerFinishTyping",
                              "onPlayerAwayChange",
                              "onPlayerMessage",
                              "onPlayerPM",
                              "onPlayerSpectate",
                              "onPlayerCrashReport"}) {
        Record(server, event);
    }
    const int32_t a = server.Connect("Alice");
    const int32_t b = server.Connect("Bob");
    const int32_t vehicle = server.CreateVehicle();  // another plugin's
    char name[24] = "Carol";
    auto& cb = server.plugin;
    CHECK(cb.OnIncomingConnection(name, sizeof(name), "pw", "1.2.3.4") == 1);
    cb.OnPlayerModuleList(a, "mods");
    CHECK(cb.OnPlayerRequestClass(a, -1) == 1);
    CHECK(cb.OnPlayerRequestSpawn(a) == 1);
    cb.OnPlayerSpawn(a);
    cb.OnPlayerUpdate(a, vcmpPlayerUpdateDriver);
    CHECK(cb.OnPlayerRequestEnterVehicle(a, vehicle, 0) == 1);
    cb.OnPlayerEnterVehicle(a, vehicle, 1);
    cb.OnPlayerExitVehicle(a, vehicle);
    cb.OnPlayerNameChange(a, "Alice", "Alicia");
    cb.OnPlayerStateChange(a, vcmpPlayerStateNormal, vcmpPlayerStateDriver);
    cb.OnPlayerActionChange(a, 1, 12);
    cb.OnPlayerOnFireChange(a, 1);
    cb.OnPlayerCrouchChange(a, 0);
    cb.OnPlayerGameKeysChange(a, 0, 0x80000000u);
    cb.OnPlayerBeginTyping(a);
    cb.OnPlayerEndTyping(a);
    cb.OnPlayerAwayChange(a, 1);
    CHECK(cb.OnPlayerMessage(a, "hello") == 1);
    CHECK(cb.OnPlayerPrivateMessage(a, b, "psst") == 1);
    cb.OnPlayerSpectate(a, -1);
    cb.OnPlayerCrashReport(a, "boom");
    const std::string v = "Vehicle(" + std::to_string(vehicle) + ")";
    const std::vector<std::string> expected = {
        "onPlayerConnection string:Carol number:24 string:pw string:1.2.3.4",
        "onPlayerModuleList Player(0) string:mods",
        "onPlayerRequestClass Player(0) number:-1",
        "onPlayerRequestSpawn Player(0)",
        "onPlayerSpawn Player(0)",
        "onPlayerUpdate Player(0) number:2",
        "onPlayerRequestEnterVehicle Player(0) " + v + " number:0",
        "onPlayerEnterVehicle Player(0) " + v + " number:1",
        "onPlayerExitVehicle Player(0) " + v,
        "onPlayerNameChange Player(0) string:Alice string:Alicia",
        "onPlayerStateChange Player(0) number:1 number:3",
        "onPlayerActionChange Player(0) number:1 number:12",
        "onPlayerFireChange Player(0) boolean:true",
        "onPlayerCrouchChange Player(0) boolean:false",
        "onPlayerGameKeysChange Player(0) number:0 number:2147483648",
        "onPlayerBeginTyping Player(0)",
        "onPlayerFinishTyping Player(0)",
        "onPlayerAwayChange Player(0) boolean:true",
        "onPlayerMessage Player(0) string:hello",
        "onPlayerPM Player(0) Player(1) string:psst",
        "onPlayerSpectate Player(0) nil:nil",
        "onPlayerCrashReport Player(0) string:boom",
    };
    CHECK(server.records == expected);
}

TEST_CASE("vehicle events") {
    FakeServer server;
    Start(server);
    for (const char* event : {"onVehicleUpdate", "onVehicleExplode", "onVehicleRespawn"}) {
        Record(server, event);
    }
    const int32_t id = server.CreateVehicle();
    server.plugin.OnVehicleUpdate(id, vcmpVehicleUpdateHealth);
    server.plugin.OnVehicleExplode(id);
    server.plugin.OnVehicleRespawn(id);
    const std::string v = "Vehicle(" + std::to_string(id) + ")";
    const std::vector<std::string> expected = {
        "onVehicleUpdate " + v + " number:4",
        "onVehicleExplode " + v,
        "onVehicleRespawn " + v,
    };
    CHECK(server.records == expected);
}

TEST_CASE("object events") {
    FakeServer server;
    Start(server);
    Record(server, "onObjectShot");
    Record(server, "onObjectTouch");
    const int32_t player = server.Connect();
    const int32_t object = server.CreateEntity(vcmpEntityPoolObject);
    server.plugin.OnObjectShot(object, player, 26);
    server.plugin.OnObjectTouched(object, player);
    const std::vector<std::string> expected = {
        "onObjectShot Object(0) Player(0) number:26",
        "onObjectTouch Object(0) Player(0)",
    };
    CHECK(server.records == expected);
}

TEST_CASE("pickup events; a pick attempt can be refused") {
    FakeServer server;
    Start(server);
    Record(server, "onPickupPickAttempt");
    Record(server, "onPickupPicked");
    Record(server, "onPickupRespawn");
    REQUIRE(server.Run(R"(
        Event.bind("onPickupPickAttempt", function(pickup, player)
            if player.id == 1 then Event.cancel() end
        end)
    )") == "");
    const int32_t a = server.Connect();
    const int32_t b = server.Connect();
    const int32_t pickup = server.CreateEntity(vcmpEntityPoolPickup);
    CHECK(server.plugin.OnPickupPickAttempt(pickup, a) == 1);
    CHECK(server.plugin.OnPickupPickAttempt(pickup, b) == 0);
    server.plugin.OnPickupPicked(pickup, a);
    server.plugin.OnPickupRespawn(pickup);
    const std::vector<std::string> expected = {
        "onPickupPickAttempt Pickup(0) Player(0)",
        "onPickupPickAttempt Pickup(0) Player(1)",
        "onPickupPicked Pickup(0) Player(0)",
        "onPickupRespawn Pickup(0)",
    };
    CHECK(server.records == expected);
}

TEST_CASE("checkpoint events") {
    FakeServer server;
    Start(server);
    Record(server, "onCheckpointEnter");
    Record(server, "onCheckpointExit");
    const int32_t player = server.Connect();
    const int32_t checkpoint = server.CreateEntity(vcmpEntityPoolCheckPoint);
    server.plugin.OnCheckpointEntered(checkpoint, player);
    server.plugin.OnCheckpointExited(checkpoint, player);
    const std::vector<std::string> expected = {
        "onCheckpointEnter Checkpoint(0) Player(0)",
        "onCheckpointExit Checkpoint(0) Player(0)",
    };
    CHECK(server.records == expected);
}

TEST_CASE("Event.cancel() makes a cancellable callback refuse") {
    FakeServer server;
    Start(server);
    const int32_t id = server.Connect();
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerMessage", function(player, text)
            if text == "spam" then Event.cancel() end
        end)
        Event.bind("onPlayerRequestSpawn", function() Event.cancel() end)
        Event.bind("onPlayerConnection", function(name) if name == "bad" then Event.cancel() end end)
    )") == "");
    CHECK(server.plugin.OnPlayerMessage(id, "spam") == 0);
    CHECK(server.plugin.OnPlayerMessage(id, "fine") == 1);
    CHECK(server.plugin.OnPlayerRequestSpawn(id) == 0);
    char bad[24] = "bad";
    char good[24] = "good";
    CHECK(server.plugin.OnIncomingConnection(bad, sizeof(bad), "", "1.1.1.1") == 0);
    CHECK(server.plugin.OnIncomingConnection(good, sizeof(good), "", "1.1.1.1") == 1);
    // Not cancellable: the return value is ignored by the server anyway.
    server.plugin.OnPlayerSpawn(id);
}

TEST_CASE("deaths: onPlayerKill with a killer, else onPlayerWasted with v1's reasons") {
    FakeServer server;
    Start(server);
    Record(server, "onPlayerKill");
    Record(server, "onPlayerWasted");
    REQUIRE(server.Run("Event.bind('onPlayerWasted', function() record('second handler') end)") ==
            "");
    const int32_t victim = server.Connect();
    const int32_t killer = server.Connect();
    server.plugin.OnPlayerDeath(victim, killer, 26, vcmpBodyPartHead);
    server.plugin.OnPlayerDeath(victim, -1, 50, vcmpBodyPartBody);
    server.plugin.OnPlayerDeath(victim, -1, 40, vcmpBodyPartBody);
    server.plugin.OnPlayerDeath(victim, -1, 39, vcmpBodyPartInVehicle);
    const std::vector<std::string> expected = {
        "onPlayerKill Player(1) Player(0) number:26 number:6",
        "onPlayerWasted Player(0) number:43",
        "second handler",
        "onPlayerWasted Player(0) number:44",
        "second handler",
        "onPlayerWasted Player(0) number:39",
        "second handler",
    };
    CHECK(server.records == expected);
}

TEST_CASE("onPlayerCommand splits the command like v1 and adds the raw text") {
    FakeServer server;
    Start(server);
    Record(server, "onPlayerCommand");
    const int32_t id = server.Connect();
    CHECK(server.plugin.OnPlayerCommand(id, "give 5 100") == 1);
    server.plugin.OnPlayerCommand(id, "  pm   Bob  hi there ");
    server.plugin.OnPlayerCommand(id, "help");
    server.plugin.OnPlayerCommand(id, "");
    server.plugin.OnPlayerCommand(id, "   ");
    const std::vector<std::string> expected = {
        "onPlayerCommand Player(0) string:give {5,100} string:5 100",
        "onPlayerCommand Player(0) string:pm {Bob,hi,there} string:Bob  hi there ",
        "onPlayerCommand Player(0) string:help nil:nil string:",
        "onPlayerCommand Player(0) nil:nil nil:nil string:",
        "onPlayerCommand Player(0) nil:nil nil:nil string:",
    };
    CHECK(server.records == expected);
}

TEST_CASE("an entity first seen in an event is adopted; callbacks after shutdown do nothing") {
    FakeServer server;
    Start(server);
    Record(server, "onPlayerSpawn");
    server.plugin.OnPlayerSpawn(7);  // no connect event was seen for 7
    CHECK(server.records == std::vector<std::string>{"onPlayerSpawn Player(7)"});
    server.Shutdown();
    CHECK(server.plugin.OnPlayerMessage(7, "late") == 1);
    server.plugin.OnPlayerSpawn(7);
    CHECK(server.records.size() == 1);
}

}  // namespace vcmp_lua::test

namespace vcmp_lua::test {

TEST_CASE("key bind events; a reload removes our binds only") {
    FakeServer server;
    REQUIRE(server.Load());
    server.key_binds[30] = {false, {5, 0, 0}};  // another plugin's, before we start
    server.Initialise();
    // Free slots answer GetKeyBindData too (keys 0): only the bind is adopted.
    CHECK(server.Eval("Bind.count()") == "1");
    REQUIRE(server.Run(R"(
        mine = Bind.create(false, 1)
        Event.bind("onPlayerKeyDown", function(player, bind)
            record("down", tostring(player), tostring(bind), bind == mine)
        end)
        Event.bind("onPlayerKeyUp", function(player, bind)
            record("up", tostring(player), tostring(bind))
        end)
    )") == "");
    const int32_t player = server.Connect();
    server.plugin.OnPlayerKeyBindDown(player, 0);
    server.plugin.OnPlayerKeyBindUp(player, 30);
    const std::vector<std::string> expected = {
        "down Player(0) Bind(0) true",
        "up Player(0) Bind(30)",
    };
    CHECK(server.records == expected);
    CHECK(server.key_binds.size() == 2);

    REQUIRE(server.Run("Server.reload()") == "");
    server.Frame();
    CHECK(server.key_binds.size() == 1);
    CHECK(server.key_binds.contains(30));
}

}  // namespace vcmp_lua::test

namespace vcmp_lua::test {

TEST_CASE("onClientData: any size is copied into a Stream of that size") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.bind("onClientData", function(player, stream, size)
            record(tostring(player), size, stream.size, stream:readNumber(), stream:readString())
        end)
        Event.bind("onClientData", function(player, stream, size)
            record("second handler", stream.remaining)
            if size > 4096 then
                record(pcall(stream.writeByte, stream, 1))
            end
        end)
    )") == "");
    const int32_t id = server.Connect();
    // 4 + 2 + 10000 bytes: v1 copied this into a 4096-byte stack array.
    std::vector<uint8_t> data = {0x2A, 0, 0, 0, 0x27, 0x10};
    data.resize(data.size() + 10000, 'z');
    server.plugin.OnClientScriptData(id, data.data(), data.size());
    REQUIRE(server.records.size() == 3);
    CHECK(server.records[0] == "Player(0) 10006 10006 42 " + std::string(10000, 'z'));
    CHECK(server.records[1] == "second handler 0");
    // A received stream larger than 4096 bytes takes no writes.
    CHECK(server.records[2].starts_with("false "));
    CHECK(server.records[2].find("no room to write a byte (10006 of 4096 bytes used)") !=
          std::string::npos);
    server.records.clear();

    // Truncated data: the handler's read fails, the server goes on.
    const uint8_t short_data[] = {1, 2};
    server.plugin.OnClientScriptData(id, short_data, sizeof(short_data));
    CHECK(server.LogText().find("not enough data to read a number") != std::string::npos);
    server.plugin.OnClientScriptData(id, nullptr, 0);
}

}  // namespace vcmp_lua::test

namespace vcmp_lua::test {

TEST_CASE("server events: plugin commands, performance reports, streaming") {
    FakeServer server;
    Start(server);
    Record(server, "onPluginCommand");
    Record(server, "onServerPerformanceReport");
    Record(server, "onEntityStreamingChange");
    REQUIRE(server.Run(R"(
        Event.bind("onPluginCommand", function(id) if id == 2 then Event.cancel() end end)
    )") == "");
    const int32_t player = server.Connect();
    CHECK(server.plugin.OnPluginCommand(1, "hello") == 1);
    CHECK(server.plugin.OnPluginCommand(2, "refused") == 0);
    const char* names[] = {"frame", "net"};
    uint64_t times[] = {15, 7};
    server.plugin.OnServerPerformanceReport(2, names, times);
    server.plugin.OnEntityStreamingChange(player, 3, vcmpEntityPoolVehicle, 1);
    const std::vector<std::string> expected = {
        "onPluginCommand number:1 string:hello",
        "onPluginCommand number:2 string:refused",
        "onServerPerformanceReport number:2 {frame,net} {15,7}",
        "onEntityStreamingChange Player(0) number:1 number:3 boolean:true",
    };
    CHECK(server.records == expected);
}

}  // namespace vcmp_lua::test
