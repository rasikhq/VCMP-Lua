#pragma once

struct lua_State;

namespace vcmp_lua {

// Registers every built-in module in package.preload: the C modules linked
// into the plugin and the embedded pure-Lua modules (plan B6). Keys match
// the names the modules require each other by ("socket.core", "socket.http",
// "luasql.postgres", ...), so `require` needs no file on disk.
//
// A lua_CFunction-style helper: call it inside a protected call, after
// luaL_openlibs.
void RegisterBuiltins(lua_State* L);

}  // namespace vcmp_lua
