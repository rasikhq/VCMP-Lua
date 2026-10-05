#include "runtime/errors.hpp"

#include <fmt/format.h>
#include <lua.hpp>

#include "runtime/runtime.hpp"

namespace vcmp_lua {

int Panic(lua_State* L) {
    // Only reads values: any allocation here could raise a second panic.
    std::string message = ErrorText(L, -1);
    if (Runtime* runtime = Runtime::FromState(L)) {
        runtime->MarkDead();
    }
    throw LuaPanic("Lua error outside a protected call: " + message);
}

int Traceback(lua_State* L) {
    // Runs inside lua_pcall: Lua errors raised here are fine, but nothing in
    // this function may own a C++ object with a destructor.
    const char* message = lua_tostring(L, 1);
    if (message == nullptr) {
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
            return 1;
        }
        message = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, message, 1);
    return 1;
}

std::string ErrorText(lua_State* L, int idx) {
    switch (lua_type(L, idx)) {
        case LUA_TSTRING: {
            std::size_t length = 0;
            const char* text = lua_tolstring(L, idx, &length);
            return std::string(text, length);
        }
        case LUA_TNUMBER:
            return lua_isinteger(L, idx) ? fmt::format("{}", lua_tointeger(L, idx))
                                         : fmt::format("{}", lua_tonumber(L, idx));
        case LUA_TBOOLEAN:
            return lua_toboolean(L, idx) ? "true" : "false";
        case LUA_TNIL:
        case LUA_TNONE:
            return "nil";
        default:
            return fmt::format("(error object is a {} value)", luaL_typename(L, idx));
    }
}

std::optional<std::string> ProtectedCall(lua_State* L, int nargs, int nresults) {
    const int base = lua_gettop(L) - nargs;
    lua_pushcfunction(L, &Traceback);
    lua_insert(L, base);
    const int status = lua_pcall(L, nargs, nresults, base);
    lua_remove(L, base);
    if (status == LUA_OK) {
        return std::nullopt;
    }
    std::string text = ErrorText(L, -1);
    lua_pop(L, 1);
    return text;
}

}  // namespace vcmp_lua
