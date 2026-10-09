// Checked binding arguments (bindings/args.hpp): conversions, ranges, and
// error messages in the style of luaL_argerror, with the script position.
#include <doctest/doctest.h>
#include <sol/sol.hpp>

#include <string>

#include "bindings/args.hpp"
#include "fake_server.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

using namespace vcmp_lua::bindings;

struct Thing {
    float value = 0;
};

// The error a Lua expression raises, called from a Lua function (so the
// binding has a name and a position).
std::string ErrorOf(FakeServer& server, const std::string& code) {
    return server.Eval("select(2, pcall(function() " + code + " end))");
}

void Start(FakeServer& server) {
    REQUIRE(server.Load());
    server.Initialise();
    sol::state_view lua(server.runtime()->state());
    lua["takes"] = [](Int32 a, Opt<Float> b) { return static_cast<double>(a) + b.value_or(0.5f); };
    lua["byte"] = [](UInt8 a) { return static_cast<int>(a); };
    lua["flag"] = [](Boolean a) { return !a; };
    lua["colour"] = [](Colour c) { return static_cast<double>(c.value); };
    lua["text"] = [](String s) { return s.value + "!"; };
    lua["vec"] = [](Vec3 v) { return v; };
    lua["vec_then"] = [](Vec3 v, String after) { return std::to_string(v.z) + after.value; };
    lua["live"] = [](Live<EntityKind::Player> player) { return player.id; };
    lua["maybe"] = [](Opt<Live<EntityKind::Player>> player) {
        return player.has_value() ? player->id : -1;
    };
    lua["fails"] = [](Ctx ctx, int code) { return Check(ctx.L, static_cast<vcmpError>(code)); };
    lua.new_usertype<Thing>(
        "Thing", "new", sol::constructors<Thing()>(),
        "set", [](Thing& thing, Float value) { thing.value = value; },
        "value", sol::property([](const Thing& thing) { return thing.value; },
                               [](Thing& thing, Float value) { thing.value = value; }));
}

}  // namespace

TEST_CASE("integer arguments: integral floats pass, ranges and fractions are refused") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("takes(2)") == "2.5");
    CHECK(server.Eval("takes(1000 / 2, 1)") == "501.0");
    CHECK(server.Eval("takes(-2147483648)") == "-2147483647.5");
    CHECK(ErrorOf(server, "takes(2147483648)") ==
          "test:1: bad argument #1 to 'takes' (value 2147483648 out of range "
          "[-2147483648, 2147483647])");
    CHECK(ErrorOf(server, "takes(1.5)") ==
          "test:1: bad argument #1 to 'takes' (number has no integer representation)");
    CHECK(ErrorOf(server, "takes(math.huge)") ==
          "test:1: bad argument #1 to 'takes' (number has no integer representation)");
    CHECK(ErrorOf(server, "takes('5')") ==
          "test:1: bad argument #1 to 'takes' (integer expected, got string)");
    CHECK(ErrorOf(server, "takes()") ==
          "test:1: bad argument #1 to 'takes' (integer expected, got no value)");
    CHECK(ErrorOf(server, "takes(1, 'x')") ==
          "test:1: bad argument #2 to 'takes' (number expected, got string)");
    CHECK(server.Eval("byte(255)") == "255");
    CHECK(ErrorOf(server, "byte(256)").find("value 256 out of range [0, 255]") !=
          std::string::npos);
    CHECK(ErrorOf(server, "byte(-1)").find("value -1 out of range [0, 255]") != std::string::npos);
}

TEST_CASE("booleans, colours and strings") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("flag(false)") == "true");
    CHECK(ErrorOf(server, "flag(0)") ==
          "test:1: bad argument #1 to 'flag' (boolean expected, got number)");
    CHECK(ErrorOf(server, "flag(nil)") ==
          "test:1: bad argument #1 to 'flag' (boolean expected, got nil)");
    CHECK(server.Eval("colour(0xFF00FFFF)") == "4278255615.0");
    CHECK(server.Eval("colour(-1)") == "4294967295.0");
    CHECK(server.Eval("colour(-2147483648)") == "2147483648.0");
    CHECK(ErrorOf(server, "colour(1 << 32)").find("out of range [-2147483648, 4294967295]") !=
          std::string::npos);
    CHECK(server.Eval("text('hi')") == "hi!");
    CHECK(server.Eval("text(12)") == "12!");
    CHECK(ErrorOf(server, "text({})") ==
          "test:1: bad argument #1 to 'text' (string expected, got table)");
}

TEST_CASE("vectors are a table or three numbers") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("table.concat(vec({1, 2.5, -3}), ' ')") == "1.0 2.5 -3.0");
    CHECK(server.Eval("table.concat(vec(1, 2.5, -3), ' ')") == "1.0 2.5 -3.0");
    CHECK(server.Eval("vec_then({1, 2, 3}, 'a')") == "3.000000a");
    CHECK(server.Eval("vec_then(1, 2, 4, 'b')") == "4.000000b");
    CHECK(ErrorOf(server, "vec({1, 2})") ==
          "test:1: bad argument #1 to 'vec' (element 3 must be a number, got nil)");
    CHECK(ErrorOf(server, "vec(1, 2)") ==
          "test:1: bad argument #3 to 'vec' (number expected, got no value)");
    CHECK(ErrorOf(server, "vec('x')") ==
          "test:1: bad argument #1 to 'vec' (table or number expected, got string)");
    // Elements are read raw: no metamethod runs inside the binding.
    CHECK(ErrorOf(server, "vec(setmetatable({}, {__index = function() return 1 end}))") ==
          "test:1: bad argument #1 to 'vec' (element 1 must be a number, got nil)");
}

TEST_CASE("methods do not count self, and property assignments name the property") {
    FakeServer server;
    Start(server);
    REQUIRE(server.Run("thing = Thing.new()") == "");
    CHECK(ErrorOf(server, "thing:set('x')") ==
          "test:1: bad argument #1 to 'set' (number expected, got string)");
    CHECK(ErrorOf(server, "thing.value = 'x'") ==
          "test:1: bad value for 'value' (number expected, got string)");
    REQUIRE(server.Run("thing.value = 2") == "");
    CHECK(server.Eval("thing.value") == "2.0");
}

TEST_CASE("entity arguments must be live handles of the right kind") {
    FakeServer server;
    Start(server);
    const int32_t id = server.Connect();
    REQUIRE(server.Run("p = test_player(" + std::to_string(id) + ") v = test_create_vehicle()") ==
            "");
    CHECK(server.Eval("live(p)") == std::to_string(id));
    CHECK(server.Eval("maybe(nil)") == "-1");
    CHECK(server.Eval("maybe(p)") == std::to_string(id));
    CHECK(ErrorOf(server, "live(nil)") ==
          "test:1: bad argument #1 to 'live' (Player expected, got nil)");
    CHECK(ErrorOf(server, "live(v)") ==
          "test:1: bad argument #1 to 'live' (Player expected, got Vehicle)");
    CHECK(ErrorOf(server, "maybe(v)") ==
          "test:1: bad argument #1 to 'maybe' (Player expected, got Vehicle)");
    server.Disconnect(id);
    CHECK(ErrorOf(server, "live(p)") == "test:1: player no longer exists");
}

TEST_CASE("server errors: refusals return false, mistakes raise") {
    FakeServer server;
    Start(server);
    CHECK(server.Eval("fails(0)") == "true");
    CHECK(server.Eval("fails(8)") == "false");  // vcmpErrorRequestDenied
    CHECK(ErrorOf(server, "fails(4)") == "test:1: 'fails' failed: argument out of bounds");
}

}  // namespace vcmp_lua::test
