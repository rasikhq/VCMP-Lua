#pragma once

#include <sol/sol.hpp>

namespace vcmp_lua::bindings {

// Registers every Lua-facing class and table in a new state.
void Register(sol::state& lua);

// The pieces of Register, one per file.
void RegisterEnums(sol::state& lua);
void RegisterEvent(sol::state& lua);
void RegisterTimer(sol::state& lua);
void RegisterEntities(sol::state& lua);
void RegisterServer(sol::state& lua);

// The function in value; throws "<where>: argument <n> must be a function
// (got <type>)" for anything else.
sol::main_protected_function RequireFunction(const sol::main_object& value, const char* where,
                                             int argument);

}  // namespace vcmp_lua::bindings
