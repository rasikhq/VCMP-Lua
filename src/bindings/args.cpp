#include "bindings/args.hpp"

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace vcmp_lua::bindings {
namespace {

// Everything here runs inside a binding, under sol2's trampoline: errors are
// C++ exceptions. The Lua API calls used cannot raise Lua errors, except on
// memory exhaustion.

bool Equals(const char* a, const char* b) noexcept {
    return a != nullptr && std::strcmp(a, b) == 0;
}

// The key of the property being read or assigned, when the running function
// is a usertype's __index/__newindex: the stack is (object, key, value).
const char* PropertyKey(lua_State* L, const lua_Debug& ar) {
    if (Equals(ar.namewhat, "metamethod") && (Equals(ar.name, "index") || Equals(ar.name, "newindex")) &&
        lua_type(L, 2) == LUA_TSTRING) {
        return lua_tostring(L, 2);
    }
    return nullptr;
}

template <EntityKind K>
bool IsHandle(lua_State* L, int index) {
    return ToHandle<K>(L, index) != nullptr;
}

using IsHandleFn = bool (*)(lua_State*, int);

template <std::size_t... I>
constexpr std::array<IsHandleFn, sizeof...(I)> MakeIsHandle(std::index_sequence<I...>) {
    return {&IsHandle<static_cast<EntityKind>(I)>...};
}
constexpr auto kIsHandle = MakeIsHandle(std::make_index_sequence<kEntityKindCount>{});

}  // namespace

std::string CurrentFunction(lua_State* L) {
    lua_Debug ar;
    if (lua_getstack(L, 0, &ar) == 0) {
        return "?";
    }
    lua_getinfo(L, "n", &ar);
    if (const char* key = PropertyKey(L, ar)) {
        return key;
    }
    return ar.name != nullptr ? ar.name : "?";
}

void ArgError(lua_State* L, int index, std::string_view message) {
    lua_Debug ar;
    if (lua_getstack(L, 0, &ar) == 0) {
        throw std::invalid_argument(fmt::format("bad argument #{} ({})", index, message));
    }
    lua_getinfo(L, "n", &ar);
    if (const char* key = PropertyKey(L, ar)) {
        throw std::invalid_argument(fmt::format("bad value for '{}' ({})", key, message));
    }
    const char* name = ar.name != nullptr ? ar.name : "?";
    if (Equals(ar.namewhat, "method")) {
        --index;  // self is not counted
        if (index == 0) {
            throw std::invalid_argument(fmt::format("calling '{}' on bad self ({})", name, message));
        }
    }
    throw std::invalid_argument(fmt::format("bad argument #{} to '{}' ({})", index, name, message));
}

void TypeError(lua_State* L, int index, std::string_view expected) {
    ArgError(L, index, fmt::format("{} expected, got {}", expected, TypeName(L, index)));
}

std::string TypeName(lua_State* L, int index) {
    const int type = lua_type(L, index);
    if (type == LUA_TNONE) {
        return "no value";
    }
    if (type == LUA_TUSERDATA) {
        for (std::size_t kind = 0; kind < kEntityKindCount; ++kind) {
            if (kIsHandle[kind](L, index)) {
                return Traits(static_cast<EntityKind>(kind)).type_name;
            }
        }
    }
    return lua_typename(L, type);
}

const char* ErrorName(vcmpError error) noexcept {
    switch (error) {
        case vcmpErrorNone:
            return "no error";
        case vcmpErrorNoSuchEntity:
            return "no such entity";
        case vcmpErrorBufferTooSmall:
            return "buffer too small";
        case vcmpErrorTooLargeInput:
            return "input too large";
        case vcmpErrorArgumentOutOfBounds:
            return "argument out of bounds";
        case vcmpErrorNullArgument:
            return "null argument";
        case vcmpErrorPoolExhausted:
            return "pool exhausted";
        case vcmpErrorInvalidName:
            return "invalid name";
        case vcmpErrorRequestDenied:
            return "request denied";
        default:
            return "unknown error";
    }
}

bool Check(lua_State* L, vcmpError error) {
    switch (error) {
        case vcmpErrorNone:
            return true;
        case vcmpErrorRequestDenied:
            return false;
        default:
            throw std::runtime_error(
                fmt::format("'{}' failed: {}", CurrentFunction(L), ErrorName(error)));
    }
}

bool CheckLast(lua_State* L, const ServerApi& api) {
    const auto last_error = VCMP_LUA_FIND(api, GetLastError);
    return last_error == nullptr || Check(L, last_error());
}

std::int64_t CheckInteger(lua_State* L, int index, std::int64_t min, std::int64_t max) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        TypeError(L, index, "integer");
    }
    std::int64_t value = 0;
    if (lua_isinteger(L, index) != 0) {
        value = lua_tointeger(L, index);
    } else {
        // Lua 5.4's / always gives a float: 1000 / 2 is 500.0.
        const lua_Number number = lua_tonumber(L, index);
        if (!std::isfinite(number) || std::floor(number) != number ||
            number < -9223372036854775808.0 || number >= 9223372036854775808.0) {
            ArgError(L, index, "number has no integer representation");
        }
        value = static_cast<std::int64_t>(number);
    }
    if (value < min || value > max) {
        ArgError(L, index, fmt::format("value {} out of range [{}, {}]", value, min, max));
    }
    return value;
}

double CheckNumber(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        TypeError(L, index, "number");
    }
    return lua_tonumber(L, index);
}

bool CheckBoolean(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TBOOLEAN) {
        TypeError(L, index, "boolean");
    }
    return lua_toboolean(L, index) != 0;
}

std::string CheckString(lua_State* L, int index) {
    const int type = lua_type(L, index);
    if (type != LUA_TSTRING && type != LUA_TNUMBER) {
        TypeError(L, index, "string");
    }
    std::size_t length = 0;
    const char* text = lua_tolstring(L, index, &length);
    return std::string(text, length);
}

std::uint32_t CheckColour(lua_State* L, int index) {
    const std::int64_t value =
        CheckInteger(L, index, std::numeric_limits<std::int32_t>::min(),
                     std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(value);
}

float TableNumber(lua_State* L, int index, int i) {
    index = lua_absindex(L, index);
    const int type = lua_rawgeti(L, index, i);
    const lua_Number number = lua_tonumber(L, -1);
    const char* type_name = lua_typename(L, type);
    lua_pop(L, 1);
    if (type != LUA_TNUMBER) {
        ArgError(L, index, fmt::format("element {} must be a number, got {}", i, type_name));
    }
    return static_cast<float>(number);
}

std::int64_t TableInteger(lua_State* L, int index, int i, std::int64_t min, std::int64_t max,
                          std::optional<std::int64_t> fallback) {
    index = lua_absindex(L, index);
    const int type = lua_rawgeti(L, index, i);
    int is_integer = 0;
    const lua_Integer value = type == LUA_TNUMBER ? lua_tointegerx(L, -1, &is_integer) : 0;
    const char* type_name = lua_typename(L, type);
    lua_pop(L, 1);
    if (type == LUA_TNIL && fallback) {
        return *fallback;
    }
    if (type != LUA_TNUMBER) {
        ArgError(L, index, fmt::format("element {} must be an integer, got {}", i, type_name));
    }
    if (is_integer == 0) {
        ArgError(L, index, fmt::format("element {} has no integer representation", i));
    }
    if (value < min || value > max) {
        ArgError(L, index,
                 fmt::format("element {}: value {} out of range [{}, {}]", i, value, min, max));
    }
    return value;
}

int TableLength(lua_State* L, int index) {
    return static_cast<int>(lua_rawlen(L, index));
}

Vec3 ArgReader::Vector(int& i) const {
    sol::stack::record tracking;
    Vec3 value = sol_lua_get(sol::types<Vec3>(), L_, index(i), tracking);
    i += tracking.used;
    return value;
}

Vec3 sol_lua_get(sol::types<Vec3>, lua_State* L, int index, sol::stack::record& tracking) {
    index = lua_absindex(L, index);
    if (lua_type(L, index) == LUA_TTABLE) {
        tracking.use(1);
        return {TableNumber(L, index, 1), TableNumber(L, index, 2), TableNumber(L, index, 3)};
    }
    tracking.use(3);
    if (lua_type(L, index) != LUA_TNUMBER) {
        TypeError(L, index, "table or number");
    }
    return {static_cast<float>(CheckNumber(L, index)), static_cast<float>(CheckNumber(L, index + 1)),
            static_cast<float>(CheckNumber(L, index + 2))};
}

int sol_lua_push(sol::types<Vec3>, lua_State* L, const Vec3& value) {
    lua_createtable(L, 3, 0);
    lua_pushnumber(L, value.x);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, value.y);
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, value.z);
    lua_rawseti(L, -2, 3);
    return 1;
}

}  // namespace vcmp_lua::bindings
