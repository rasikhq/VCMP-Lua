// Scheduler (plan B3.8): 64-bit millisecond timers on a fake clock.
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "fake_server.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

void Start(FakeServer& server) {
    REQUIRE(server.Load());
    server.Initialise();
}

}  // namespace

TEST_CASE("timers keep running across a simulated 72-minute clock") {
    FakeServer server;
    Start(server);
    // v1 kept microseconds in 32 bits: its timers stalled after ~71.6 minutes.
    // The fake clock starts right below that point.
    REQUIRE(server.Run(R"(
        ticks = 0
        last_minute = 0
        Timer.create(function() ticks = ticks + 1 end, 1000, -1)
    )") == "");
    constexpr int kFrames = 72 * 60 * 10;  // 72 minutes of 100 ms frames
    for (int frame = 0; frame < kFrames; ++frame) {
        server.Frame(100);
    }
    CHECK(server.Eval("ticks") == std::to_string(72 * 60));
}

TEST_CASE("Timer.create checks its arguments") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("select(2, pcall(Timer.create, 42, 100, 1))")
              .find("Timer.create: argument 1 must be a function (got number)") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(Timer.create, print, -1, 1))")
              .find("timer interval must be between 0 and 2^40 milliseconds") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(Timer.create, print, 100, 0))")
              .find("timer repeat count must be -1 (forever) or positive") != std::string::npos);
    CHECK(server.Eval("select(2, pcall(Timer.create, print, 100.5, 1))")
              .find("Timer.create: argument 2 (interval) must be an integer (got 100.5)") !=
          std::string::npos);
    CHECK(server.Eval("select(2, pcall(Timer.create, print, '100', 1))")
              .find("must be an integer (got string)") != std::string::npos);
    CHECK(server.Eval("pcall(Timer.create, print, 1000 / 2, 1)") == "true");
}

TEST_CASE("repeat counts, arguments, thisTimer and destroy") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        three = Timer.create(function(a, b)
            record("three", a, b, thisTimer == three)
        end, 250, 3, "x", 7)
        forever = Timer.create(function()
            record("forever")
            if thisTimer:destroy() then record("destroyed itself") end
            record("active", thisTimer.active)
        end, 100, -1)
    )") == "");
    for (int frame = 0; frame < 20; ++frame) {
        server.Frame(100);
    }
    const std::vector<std::string> expected = {
        "forever", "destroyed itself", "active false",
        "three x 7 true", "three x 7 true", "three x 7 true",
    };
    CHECK(server.records == expected);
    CHECK(server.Eval("three.active") == "false");
    CHECK(server.Eval("tostring(three)") == "Timer(1)");
    CHECK(server.Eval("Timer.destroy(three)") == "false");
    CHECK(server.Eval("thisTimer") == "nil");
    CHECK(server.runtime()->Timers().Count() == 0);
}

TEST_CASE("timers created or destroyed inside a timer callback wait for the next tick") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        Timer.create(function()
            record("creator")
            -- Interval 0: due at once, but not in the tick that created it.
            Timer.create(function() record("child") end, 0, 1)
            -- Due in this tick too (same due time, later id), but destroyed
            -- before its turn.
            record("destroyed", victim:destroy())
        end, 100, 1)
        victim = Timer.create(function() record("victim") end, 100, -1)
    )") == "");
    server.Frame(100);
    const std::vector<std::string> first = {"creator", "destroyed true"};
    CHECK(server.records == first);
    server.Frame(1);
    const std::vector<std::string> second = {"creator", "destroyed true", "child"};
    CHECK(server.records == second);
    server.Frame(100);
    CHECK(server.records == second);
    CHECK(server.runtime()->Timers().Count() == 0);
}

TEST_CASE("a timer that falls behind runs once, then keeps its rate") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run("count = 0 Timer.create(function() count = count + 1 end, 100, -1)") == "");
    server.Frame(10'000);  // a 10 s stall
    CHECK(server.Eval("count") == "1");
    server.Frame(50);
    CHECK(server.Eval("count") == "1");
    server.Frame(50);
    CHECK(server.Eval("count") == "2");
}

TEST_CASE("a timer callback that errors is logged and the timer keeps going") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        n = 0
        Timer.create(function() n = n + 1 error("tick failed") end, 100, 2)
    )") == "");
    server.Frame(100);
    server.Frame(100);
    CHECK(server.Eval("n") == "2");
    CHECK(server.LogText().find("Timer callback: ") != std::string::npos);
    CHECK(server.LogText().find("tick failed") != std::string::npos);
}

TEST_CASE("a timer created inside a coroutine survives it") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run(R"(
        local co = coroutine.create(function()
            local tag = { name = "from coroutine" }
            Timer.create(function(t) record(t.name) end, 100, 1, tag)
            coroutine.yield()
        end)
        assert(coroutine.resume(co))
        co = nil
        collectgarbage()
        collectgarbage()
    )") == "");
    server.Frame(100);
    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0] == "from coroutine");
}

}  // namespace vcmp_lua::test
