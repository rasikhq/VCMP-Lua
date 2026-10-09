#include "runtime/preload.hpp"

#include <lua.hpp>

#include <array>
#include <string_view>

#include <sol/sol.hpp>

#include "runtime/embedded.hpp"

// luaopen_* functions of the C modules in cmake/deps.
extern "C" {
int luaopen_lfs(lua_State* L);
int luaopen_cjson(lua_State* L);
int luaopen_cjson_safe(lua_State* L);
int luaopen_socket_core(lua_State* L);
int luaopen_mime_core(lua_State* L);
int luaopen_luasql_sqlite3(lua_State* L);
int luaopen_luasql_postgres(lua_State* L);
int luaopen_luasql_mysql(lua_State* L);
}

namespace vcmp_lua {
namespace {

struct CModule {
    const char* name;
    lua_CFunction open;
};

constexpr std::array<CModule, 8> kCModules = {{
    {"lfs", luaopen_lfs},
    {"cjson", luaopen_cjson},
    {"cjson.safe", luaopen_cjson_safe},
    {"socket.core", luaopen_socket_core},
    {"mime.core", luaopen_mime_core},
    {"luasql.sqlite3", luaopen_luasql_sqlite3},
    {"luasql.postgres", luaopen_luasql_postgres},
    {"luasql.mysql", luaopen_luasql_mysql},
}};

// package.preload loader of an embedded module. Upvalue 1: its index in
// embedded::Modules(). Runs inside `require`, so it owns no C++ objects.
int LoadEmbedded(lua_State* L) {
    const auto index = static_cast<std::size_t>(lua_tointeger(L, lua_upvalueindex(1)));
    const embedded::Module& module = embedded::Modules()[index];
    const int status = luaL_loadbufferx(L, reinterpret_cast<const char*>(module.source),
                                        module.size, module.chunkname, "t");
    if (status != LUA_OK) {
        return lua_error(L);
    }
    lua_pushvalue(L, 1);  // module name
    lua_pushvalue(L, 2);  // loader data
    lua_call(L, 2, 1);
    return 1;
}

// Embedded files under this prefix are the runtime's own and not modules.
constexpr std::string_view kInternalPrefix = "vcmp-lua/";

bool Internal(const embedded::Module& module) {
    return std::string_view(module.name).starts_with(kInternalPrefix);
}

// package.preload loader of PreloadValue: returns upvalue 1.
int ReturnUpvalue(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

}  // namespace

void RegisterBuiltins(lua_State* L) {
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    for (const CModule& module : kCModules) {
        lua_pushcfunction(L, module.open);
        lua_setfield(L, -2, module.name);
    }
    const auto modules = embedded::Modules();
    for (std::size_t i = 0; i < modules.size(); ++i) {
        if (Internal(modules[i])) {
            continue;
        }
        lua_pushinteger(L, static_cast<lua_Integer>(i));
        lua_pushcclosure(L, &LoadEmbedded, 1);
        lua_setfield(L, -2, modules[i].name);
    }
    lua_pop(L, 1);
}

void RunPrelude(lua_State* L) {
    for (const embedded::Module& module : embedded::Modules()) {
        if (std::string_view(module.name) != "vcmp-lua/prelude") {
            continue;
        }
        if (luaL_loadbufferx(L, reinterpret_cast<const char*>(module.source), module.size,
                             module.chunkname, "t") != LUA_OK) {
            lua_error(L);
        }
        lua_call(L, 0, 1);
        return;
    }
    luaL_error(L, "the prelude is not embedded");
}

void PreloadValue(sol::state& lua, const char* name, const sol::object& value) {
    lua_State* L = lua.lua_state();
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    value.push(L);
    lua_pushcclosure(L, &ReturnUpvalue, 1);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

}  // namespace vcmp_lua
