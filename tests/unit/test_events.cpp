// EventBus: bind, unbind and cancel during nested dispatch,
// handlers that error, handlers bound inside coroutines.
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

}  // namespace

TEST_CASE("the Event API checks its arguments") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("select(2, pcall(Event.bind, 'onNoSuchEvent', print))")
              .find("unknown event 'onNoSuchEvent'") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(Event.bind, 'onPlayerConnect', 42))")
              .find("Event.bind: argument 2 must be a function (got number)") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(Event.cancel))")
              .find("Event.cancel() called outside an event handler") != std::string::npos);

    REQUIRE(server.Run("function handler() end") == "");
    CHECK(server.Eval("Event.bind('onPlayerConnect', handler)") == "true");
    CHECK(server.Eval("Event.bind('onPlayerConnect', handler)") == "false");  // already bound
    CHECK(server.Eval("Event.unbind('onPlayerConnect', handler)") == "true");
    CHECK(server.Eval("Event.unbind('onPlayerConnect', handler)") == "false");

    CHECK(server.Eval("Event.create('onCustom')") == "true");
    CHECK(server.Eval("Event.create('onCustom')") == "false");
    CHECK(server.Eval("Event.create('onPlayerConnect')") == "false");
    CHECK(server.Eval("Event.trigger('onCustom', 1, 2)") == "true");  // no handlers
}

TEST_CASE("bind, unbind and cancel during nested dispatch") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.create("outer")
        Event.create("inner")

        local function late() record("late") end
        local function third() record("third") end

        Event.bind("outer", function(a, b)
            record("first", a, b)
            -- Bound during the dispatch: runs from the next dispatch on.
            Event.bind("outer", late)
            -- Unbound during the dispatch, before its turn: does not run
            -- (checked by the first dispatch, which is not cancelled).
            Event.unbind("outer", third)
            -- A nested dispatch has its own cancel flag.
            record("inner returned", Event.trigger("inner", "x"))
        end)
        cancel_outer = true
        Event.bind("outer", function()
            record("second")
            if cancel_outer then Event.cancel() end
            record("second continues")
        end)
        Event.bind("outer", third)

        Event.bind("inner", function(x)
            record("inner", x)
            Event.cancel()
        end)
        Event.bind("inner", function() record("inner after cancel") end)
    )") == "");

    // Not cancelled: "third" was unbound before its turn, "late" was bound
    // during the dispatch; neither runs.
    REQUIRE(server.Run("cancel_outer = false") == "");
    CHECK(server.Eval("Event.trigger('outer', 1, 'two')") == "true");
    const std::vector<std::string> expected = {
        "first 1 two", "inner x", "inner returned false", "second", "second continues",
    };
    CHECK(server.records == expected);

    // Next dispatch: "late" runs now.
    server.records.clear();
    CHECK(server.Eval("Event.trigger('outer', 3, 4)") == "true");
    const std::vector<std::string> expected_next = {
        "first 3 4", "inner x", "inner returned false", "second", "second continues", "late",
    };
    CHECK(server.records == expected_next);

    // Cancelled by "second": "late" does not run, and trigger returns false.
    server.records.clear();
    REQUIRE(server.Run("cancel_outer = true") == "");
    CHECK(server.Eval("Event.trigger('outer', 5, 6)") == "false");
    const std::vector<std::string> expected_cancelled = {
        "first 5 6", "inner x", "inner returned false", "second", "second continues",
    };
    CHECK(server.records == expected_cancelled);
}

TEST_CASE("handlers added during a dispatch are not called by it; unbinding self works") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.create("e")
        local count = 0
        local function self_removing()
            record("once")
            Event.unbind("e", self_removing)
        end
        local function adder()
            count = count + 1
            record("adder " .. count)
            Event.bind("e", function() record("added by " .. count) end)
        end
        Event.bind("e", self_removing)
        Event.bind("e", adder)
    )") == "");
    REQUIRE(server.Run("Event.trigger('e') Event.trigger('e')") == "");
    const std::vector<std::string> expected = {"once", "adder 1", "adder 2", "added by 2"};
    CHECK(server.records == expected);
}

TEST_CASE("a kick inside a handler dispatches the disconnect re-entrantly") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Event.bind("onPlayerConnect", function(player)
            test_kick(player)
            record("after kick", (pcall(function() return player.id end)))
        end)
        Event.bind("onPlayerDisconnect", function(player, reason)
            record("disconnect", player.id, reason)
        end)
    )") == "");
    const int32_t id = server.Connect();
    CHECK_FALSE(server.Connected(id));
    REQUIRE(server.records.size() == 2);
    CHECK(server.records[0] == "disconnect " + std::to_string(id) + " 2");
    CHECK(server.records[1] == "after kick false");
}

TEST_CASE("a handler that errors is logged with a traceback, and the next one runs") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        local function broken() error("handler exploded") end
        Event.bind("onPlayerConnect", function() broken() end)
        Event.bind("onPlayerConnect", function(player) record("second", player.id) end)
    )") == "");
    server.Connect();
    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0] == "second 0");
    const std::string log = server.LogText();
    CHECK(log.find("onPlayerConnect: ") != std::string::npos);
    CHECK(log.find("handler exploded") != std::string::npos);
    CHECK(log.find("stack traceback") != std::string::npos);
    CHECK(log.find("in upvalue 'broken'") != std::string::npos);
}

TEST_CASE("a handler bound inside a coroutine survives the coroutine") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        local co = coroutine.create(function(tag)
            Event.bind("onPlayerConnect", function(player)
                record("connected", tag, player.id)
            end)
            Event.create("fromCoroutine")
            Event.bind("fromCoroutine", function(...)
                record("trigger", ...)
                record("on the main thread", select(2, coroutine.running()))
            end)
            coroutine.yield()
        end)
        assert(coroutine.resume(co, "tagged"))
        co = nil
        collectgarbage()
        collectgarbage()
    )") == "");
    server.Connect();
    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0] == "connected tagged 0");

    // Triggered from inside another coroutine: dispatched on that thread.
    REQUIRE(server.Run(R"(
        local co = coroutine.wrap(function() return Event.trigger("fromCoroutine", 1, "a") end)
        record("returned", co())
    )") == "");
    REQUIRE(server.records.size() == 4);
    CHECK(server.records[1] == "trigger 1 a");
    CHECK(server.records[2] == "on the main thread false");
    CHECK(server.records[3] == "returned true");
}

}  // namespace vcmp_lua::test
