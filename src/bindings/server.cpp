// Server: the plugin-level functions. Phase 3 ports the rest of v1's Server.
#include <sol/sol.hpp>

#include "bindings/bindings.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {

void RegisterServer(sol::state& lua) {
    sol::table server = lua.create_named_table("Server");

    // Server.reload(): at the end of the frame the plugin deletes what the
    // scripts created, closes the Lua state, reads luaconfig.lua again and
    // runs the scripts in a new state.
    server["reload"] = [](sol::this_state L) { Runtime::Require(L).RequestReload(); };
}

}  // namespace vcmp_lua::bindings
