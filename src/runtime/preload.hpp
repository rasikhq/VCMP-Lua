#pragma once

#include <sol/forward.hpp>

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

// Runs lua/prelude.lua (the module sandbox) and leaves the table it returns
// on the stack. Same calling rules as RegisterBuiltins, after it.
void RunPrelude(lua_State* L);

// package.preload[name] returns value: a module made by the C++ bindings
// (require "http", require "hash"). Called while the bindings register.
void PreloadValue(sol::state& lua, const char* name, const sol::object& value);

}  // namespace vcmp_lua
