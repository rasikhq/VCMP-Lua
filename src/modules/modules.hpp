#pragma once

#include <sol/sol.hpp>

namespace vcmp_lua::modules {

// The global Hash table, also require "hash".
void RegisterHash(sol::state& lua);

// require "http": requests over the runtime's Http (modules/http.hpp).
void RegisterHttp(sol::state& lua);

}  // namespace vcmp_lua::modules
