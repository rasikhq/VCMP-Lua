#pragma once

// Checked binding arguments and the errors bindings raise.
//
// A binding declares what it accepts with the types below instead of plain
// int/float/bool. Each type converts one Lua argument (Vec3: one table or
// three numbers) and raises an error in the style of luaL_argerror when the
// value does not fit. The error is a C++ exception, which sol2's trampoline
// turns into a Lua error; the exception handler installed by
// bindings::Register adds the "main.lua:12:" position. Nothing here calls
// lua_error.

#include <vcmp.h>
#include <lua.hpp>
#include <sol/sol.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "core/entity_pool.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {

// The Lua-facing name of the function running on L: 'setWeapon' for a
// method, 'health' for a property assignment, or '?'.
std::string CurrentFunction(lua_State* L);

// Throws "bad argument #<index> to '<function>' (<message>)", counting like
// luaL_argerror: a method's self is not counted, and a property assignment
// says "bad value for '<property>' (<message>)".
[[noreturn]] void ArgError(lua_State* L, int index, std::string_view message);

// Throws ArgError with "<expected> expected, got <type of the argument>".
[[noreturn]] void TypeError(lua_State* L, int index, std::string_view expected);

// The type of the value at index for messages: "nil", "string", "Vehicle",
// "no value", ...
std::string TypeName(lua_State* L, int index);

// The text of a server error code: "argument out of bounds".
const char* ErrorName(vcmpError error) noexcept;

// Checks the result of a server call. True for vcmpErrorNone, false for
// vcmpErrorRequestDenied (the server refused, e.g. a player who is not
// spawned); any other error is a mistake in the arguments and throws
// "'<function>' failed: <error>".
bool Check(lua_State* L, vcmpError error);

// Check() on the server's last error, for functions that report errors only
// through GetLastError.
bool CheckLast(lua_State* L, const ServerApi& api);

// The server function `field`, for a binding whose self (or context) has an
// api(); raises "<field> is not supported by this server version" if missing.
#define VCMP_FN(self, field) VCMP_LUA_API((self).api(), field)

// An integer, or a float with an integral value, within [min, max].
std::int64_t CheckInteger(lua_State* L, int index, std::int64_t min, std::int64_t max);
double CheckNumber(lua_State* L, int index);
// Only true and false: 0 is true in Lua, so numbers are refused.
bool CheckBoolean(lua_State* L, int index);
// A string, or a number (converted, like luaL_checklstring).
std::string CheckString(lua_State* L, int index);
// An RGB(A) colour as 32 bits: [-2^31, 2^32), so both 0xFF0000FF and the
// same bits as a negative int32 are accepted.
std::uint32_t CheckColour(lua_State* L, int index);

template <typename T>
struct Int {
    static_assert(std::is_integral_v<T> && sizeof(T) <= 4);
    T value;
    operator T() const noexcept { return value; }
};
using Int32 = Int<std::int32_t>;
using UInt32 = Int<std::uint32_t>;
using Int16 = Int<std::int16_t>;
using UInt16 = Int<std::uint16_t>;
using UInt8 = Int<std::uint8_t>;

struct Float {
    float value;
    operator float() const noexcept { return value; }
};

struct Double {
    double value;
    operator double() const noexcept { return value; }
};

struct Boolean {
    bool value;
    operator bool() const noexcept { return value; }
};

struct String {
    std::string value;
    [[nodiscard]] const char* c_str() const noexcept { return value.c_str(); }
};

struct Colour {
    std::uint32_t value;
    operator std::uint32_t() const noexcept { return value; }
};

// A position or direction. As an argument: a table {x, y, z} (one argument)
// or three numbers. As a result: a table {x, y, z}, as in 2.x.
struct Vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
};

// An optional argument: nil or missing gives an empty value.
template <typename T>
struct Opt {
    std::optional<T> value;
    [[nodiscard]] bool has_value() const noexcept { return value.has_value(); }
    [[nodiscard]] const T& operator*() const { return *value; }
    [[nodiscard]] const T* operator->() const { return &*value; }
    // The converted value, or fallback (for the types with a .value member).
    template <typename U>
    [[nodiscard]] U value_or(U fallback) const {
        return value ? static_cast<U>(value->value) : fallback;
    }
};

// The usable runtime and the running thread, for bindings without an entity
// self (Server.*, Vehicle.create, ...). Takes no Lua argument; raises
// "runtime shutting down" when the runtime is closing.
struct Ctx {
    Runtime* runtime;
    lua_State* L;

    [[nodiscard]] const ServerApi& api() const noexcept { return runtime->api(); }
};

// A live entity of kind K: the argument must be a handle of that kind whose
// entity still exists ("vehicle no longer exists" otherwise). Used for self
// in every entity method, so no method can forget the check.
template <EntityKind K>
struct Live {
    Runtime* runtime;
    lua_State* L;  // the running thread
    std::int32_t id;
    std::uint32_t generation;

    [[nodiscard]] const ServerApi& api() const noexcept { return runtime->api(); }
    [[nodiscard]] EntityPool& pool() const noexcept { return runtime->Entities().Get(K); }
    [[nodiscard]] EntityRef<K> ref() const noexcept { return {id, generation}; }
};

// The handle at index if it is one of kind K (alive or not), else null.
template <EntityKind K>
const EntityHandle<K>* ToHandle(lua_State* L, int index) {
    if (lua_type(L, index) != LUA_TUSERDATA ||
        !sol::stack::check<EntityHandle<K>>(L, index, &sol::no_panic)) {
        return nullptr;
    }
    return &sol::stack::get<EntityHandle<K>&>(L, index);
}

// Key binds raise no pool events: before use, the server is asked whether
// the bind still exists; if not, it is released and "bind no longer exists"
// raised.
void RequireBindExists(Runtime& runtime, std::int32_t id);

template <EntityKind K>
Live<K> CheckLive(lua_State* L, int index) {
    Runtime& runtime = Runtime::Require(L);
    const EntityHandle<K>* handle = ToHandle<K>(L, index);
    if (handle == nullptr) {
        TypeError(L, index, Traits(K).type_name);
    }
    runtime.Entities().Get(K).Require(handle->id, handle->generation);
    if constexpr (K == EntityKind::Bind) {
        RequireBindExists(runtime, handle->id);
    }
    return {&runtime, L, handle->id, handle->generation};
}

// sol2 customization points. The checks accept everything and only count
// stack slots; the getters validate and throw. Bindings therefore never use
// sol::overload with these types: a binding that accepts several forms
// inspects its arguments itself.

template <typename T, typename Handler>
bool sol_lua_check(sol::types<Int<T>>, lua_State*, int, Handler&&, sol::stack::record& tracking) {
    tracking.use(1);
    return true;
}
template <typename T>
Int<T> sol_lua_get(sol::types<Int<T>>, lua_State* L, int index, sol::stack::record& tracking) {
    tracking.use(1);
    return {static_cast<T>(
        CheckInteger(L, index, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()))};
}

#define VCMP_LUA_SIMPLE_ARG(Type, expression)                          \
    template <typename Handler>                                        \
    bool sol_lua_check(sol::types<Type>, lua_State*, int, Handler&&,   \
                       sol::stack::record& tracking) {                 \
        tracking.use(1);                                               \
        return true;                                                   \
    }                                                                  \
    inline Type sol_lua_get(sol::types<Type>, lua_State* L, int index, \
                            sol::stack::record& tracking) {            \
        tracking.use(1);                                               \
        return {expression};                                           \
    }

VCMP_LUA_SIMPLE_ARG(Float, static_cast<float>(CheckNumber(L, index)))
VCMP_LUA_SIMPLE_ARG(Double, CheckNumber(L, index))
VCMP_LUA_SIMPLE_ARG(Boolean, CheckBoolean(L, index))
VCMP_LUA_SIMPLE_ARG(String, CheckString(L, index))
VCMP_LUA_SIMPLE_ARG(Colour, CheckColour(L, index))
#undef VCMP_LUA_SIMPLE_ARG

// A table uses one slot, three numbers use three.
template <typename Handler>
bool sol_lua_check(sol::types<Vec3>, lua_State* L, int index, Handler&&,
                   sol::stack::record& tracking) {
    tracking.use(lua_type(L, index) == LUA_TTABLE ? 1 : 3);
    return true;
}
Vec3 sol_lua_get(sol::types<Vec3>, lua_State* L, int index, sol::stack::record& tracking);
int sol_lua_push(sol::types<Vec3>, lua_State* L, const Vec3& value);

template <typename T, typename Handler>
bool sol_lua_check(sol::types<Opt<T>>, lua_State* L, int index, Handler&& handler,
                   sol::stack::record& tracking) {
    if (lua_isnoneornil(L, index)) {
        tracking.use(1);
        return true;
    }
    return sol::stack::check<T>(L, index, std::forward<Handler>(handler), tracking);
}
template <typename T>
Opt<T> sol_lua_get(sol::types<Opt<T>>, lua_State* L, int index, sol::stack::record& tracking) {
    if (lua_isnoneornil(L, index)) {
        tracking.use(1);
        return {};
    }
    return {sol::stack::get<T>(L, index, tracking)};
}

template <typename Handler>
bool sol_lua_check(sol::types<Ctx>, lua_State*, int, Handler&&, sol::stack::record& tracking) {
    tracking.use(0);
    return true;
}
inline Ctx sol_lua_get(sol::types<Ctx>, lua_State* L, int, sol::stack::record& tracking) {
    tracking.use(0);
    return {&Runtime::Require(L), L};
}

template <EntityKind K, typename Handler>
bool sol_lua_check(sol::types<Live<K>>, lua_State*, int, Handler&&, sol::stack::record& tracking) {
    tracking.use(1);
    return true;
}
template <EntityKind K>
Live<K> sol_lua_get(sol::types<Live<K>>, lua_State* L, int index, sol::stack::record& tracking) {
    tracking.use(1);
    return CheckLive<K>(L, lua_absindex(L, index));
}

// For bindings that accept several forms (a table or numbers, optional
// arguments in the middle): reads the arguments of the running function by
// position, with the same checks and messages as the argument types.
class ArgReader {
public:
    // first: the stack index of argument 1.
    ArgReader(lua_State* L, int first) noexcept : L_(L), first_(first) {}

    [[nodiscard]] lua_State* L() const noexcept { return L_; }
    [[nodiscard]] int index(int i) const noexcept { return first_ + i - 1; }
    [[nodiscard]] int type(int i) const noexcept { return lua_type(L_, index(i)); }
    [[nodiscard]] bool missing(int i) const noexcept { return lua_isnoneornil(L_, index(i)); }
    // Number of arguments from argument 1 on.
    [[nodiscard]] int count() const noexcept { return lua_gettop(L_) - first_ + 1; }

    template <typename T>
    T Int(int i) const {
        return static_cast<T>(CheckInteger(L_, index(i), std::numeric_limits<T>::min(),
                                           std::numeric_limits<T>::max()));
    }
    template <typename T>
    T IntOr(int i, T fallback) const {
        return missing(i) ? fallback : Int<T>(i);
    }
    [[nodiscard]] float Number(int i) const {
        return static_cast<float>(CheckNumber(L_, index(i)));
    }
    [[nodiscard]] float NumberOr(int i, float fallback) const {
        return missing(i) ? fallback : Number(i);
    }
    [[nodiscard]] bool Bool(int i) const { return CheckBoolean(L_, index(i)); }
    [[nodiscard]] bool BoolOr(int i, bool fallback) const {
        return missing(i) ? fallback : Bool(i);
    }
    [[nodiscard]] std::uint32_t ColourAt(int i) const { return CheckColour(L_, index(i)); }

    // A Vec3 starting at argument i: a table (one argument) or three
    // numbers. i is advanced past it.
    Vec3 Vector(int& i) const;

private:
    lua_State* L_;
    int first_;
};

// Element i (1-based) of the table at index as a number; raises "bad
// argument" when it is missing or not a number. Uses raw access.
float TableNumber(lua_State* L, int index, int i);

// Element i (1-based) of the table at index as an integer in [min, max];
// nil gives fallback when one is passed, else raises "bad argument".
std::int64_t TableInteger(lua_State* L, int index, int i, std::int64_t min, std::int64_t max,
                          std::optional<std::int64_t> fallback = std::nullopt);

// Pushes t[key] of the table at index without metamethods and returns its
// type.
int RawField(lua_State* L, int index, const char* key);

// The number of array elements of the table at index (raw length).
int TableLength(lua_State* L, int index);

}  // namespace vcmp_lua::bindings
