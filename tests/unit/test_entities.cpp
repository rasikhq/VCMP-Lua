// Entity handles: identity, invalidation, and adopt/release during
// the synchronous pool events of Create*/Delete*.
#include <doctest/doctest.h>

#include "core/entity_pool.hpp"
#include "fake_server.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

void Start(FakeServer& server) {
    REQUIRE(server.Load());
    server.Initialise();
}

}  // namespace

TEST_CASE("the same entity is always the same Lua value, until it goes") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerConnect", function(player) joined = joined or player end)
    )") == "");
    const int32_t id = server.Connect();
    CHECK(server.Eval("joined.id") == std::to_string(id));
    CHECK(server.Eval("rawequal(joined, test_player(joined.id))") == "true");
    CHECK(server.Eval("joined == test_player(joined.id)") == "true");
    REQUIRE(server.Run(R"(
        seen = { [joined] = "yes" }
        joined.data.score = 10
    )") == "");
    CHECK(server.Eval("seen[test_player(joined.id)]") == "yes");
    CHECK(server.Eval("test_player(joined.id).data.score") == "10");
    CHECK(server.Eval("tostring(joined)") == "Player(" + std::to_string(id) + ")");
    CHECK(server.Eval("test_player(-1)") == "nil");
    CHECK(server.Eval("test_player(5)") == "nil");  // not connected

    server.Disconnect(id);
    CHECK(server.Eval("select(2, pcall(function() return joined.id end))")
              .find("player no longer exists") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(function() return joined.data end))")
              .find("player no longer exists") != std::string::npos);
    CHECK(server.Eval("tostring(joined)") ==
          "Player(" + std::to_string(id) + ", no longer exists)");

    // The id comes back at once (docs/internals.md); the old handle stays dead.
    REQUIRE(server.Connect() == id);
    CHECK(server.Eval("joined == test_player(" + std::to_string(id) + ")") == "false");
    CHECK(server.Eval("test_player(" + std::to_string(id) + ").data.score") == "nil");
    CHECK(server.Eval("pcall(function() return joined.id end)") == "false");
}

TEST_CASE("a handle is valid during the disconnect event and dead after it") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerDisconnect", function(player, reason)
            record("disconnect", player.id, reason)
            last = player
        end)
    )") == "");
    const int32_t id = server.Connect();
    server.Disconnect(id, vcmpDisconnectReasonQuit);
    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0] == "disconnect " + std::to_string(id) + " 1");
    CHECK(server.Eval("pcall(function() return last.id end)") == "false");
}

TEST_CASE("adopt and release are idempotent during synchronous pool events") {
    FakeServer server;
    Start(server);
    // The pool event arrives while test_create_vehicle is still inside
    // CreateVehicle; the handler sees the vehicle already adopted.
    REQUIRE(server.Run(R"(
        Event.bind("onEntityPoolChange", function(pool, id, deleted)
            local vehicle = test_vehicle(id)
            record("pool", pool, id, deleted, vehicle ~= nil)
            in_event = vehicle
        end)
        vehicle = test_create_vehicle()
    )") == "");
    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0] == "pool 1 1 false true");
    CHECK(server.Eval("rawequal(vehicle, in_event)") == "true");
    CHECK(server.Eval("vehicle.id") == "1");

    EntityPool& vehicles = server.runtime()->Entities().Get(EntityKind::Vehicle);
    const uint32_t generation = vehicles.Ref<EntityKind::Vehicle>(1).generation;
    vehicles.Adopt(1);  // again: a no-op
    CHECK(vehicles.Ref<EntityKind::Vehicle>(1).generation == generation);
    CHECK(server.Eval("rawequal(vehicle, test_vehicle(1))") == "true");

    // Deleting: valid during the event, released by the event, and the
    // binding's own release after DeleteVehicle is a no-op.
    REQUIRE(server.Run("test_delete_vehicle(vehicle)") == "");
    REQUIRE(server.records.size() == 2);
    CHECK(server.records[1] == "pool 1 1 true true");
    CHECK_FALSE(server.VehicleExists(1));
    CHECK_FALSE(vehicles.Alive(1));
    vehicles.Release(1);
    CHECK(server.Eval("pcall(function() return vehicle.id end)") == "false");
    CHECK(server.Eval("select(2, pcall(test_delete_vehicle, vehicle))")
              .find("vehicle no longer exists") != std::string::npos);

    // The id is reused with exactly one more generation.
    REQUIRE(server.Run("again = test_create_vehicle()") == "");
    CHECK(server.Eval("again.id") == "1");
    CHECK(vehicles.Ref<EntityKind::Vehicle>(1).generation == generation + 1);
    CHECK(server.Eval("again == vehicle") == "false");
}

TEST_CASE("other plugins' entities are adopted, also those created before the scripts") {
    FakeServer server;
    REQUIRE(server.Load());
    const int32_t early = server.CreateVehicle();  // before OnServerInitialise
    const int32_t player = server.Connect();
    server.Initialise();
    CHECK(server.Eval("test_vehicle(" + std::to_string(early) + ") ~= nil") == "true");
    CHECK(server.Eval("test_player(" + std::to_string(player) + ") ~= nil") == "true");

    const int32_t late = server.CreateVehicle();
    CHECK(server.Eval("test_vehicle(" + std::to_string(late) + ").id") == std::to_string(late));
    server.DeleteVehicle(late);
    CHECK(server.Eval("test_vehicle(" + std::to_string(late) + ")") == "nil");
}

TEST_CASE("ids outside a pool are ignored") {
    FakeServer server;
    Start(server);
    EntityPool& vehicles = server.runtime()->Entities().Get(EntityKind::Vehicle);
    vehicles.Adopt(0);  // vehicle ids start at 1
    vehicles.Adopt(5000);
    CHECK_FALSE(vehicles.Alive(0));
    CHECK_FALSE(vehicles.Alive(5000));
    CHECK(server.LogText().find("outside the pool") != std::string::npos);
    CHECK(server.Eval("test_vehicle(5000)") == "nil");
}

}  // namespace vcmp_lua::test
