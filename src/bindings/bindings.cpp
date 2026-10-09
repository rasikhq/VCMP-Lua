#include "bindings/bindings.hpp"

#include <fmt/format.h>
#include <lua.hpp>
#include <sol/sol.hpp>

#include <stdexcept>

namespace vcmp_lua::bindings {
namespace {

// sol2 turns a C++ exception thrown by a binding into a Lua error with this
// message. It adds the script position, as luaL_error does: level 1 is the
// Lua code that called the binding.
int ExceptionHandler(lua_State* L, sol::optional<const std::exception&>, sol::string_view what) {
    luaL_where(L, 1);
    lua_pushlstring(L, what.data(), what.size());
    lua_concat(L, 2);
    return 1;
}

}  // namespace

void Register(sol::state& lua) {
    lua.set_exception_handler(&ExceptionHandler);
    RegisterEnums(lua);
    RegisterEvent(lua);
    RegisterTimer(lua);
    RegisterEntities(lua);
    RegisterServer(lua);
    RegisterStream(lua);
}

sol::main_protected_function RequireFunction(const sol::main_object& value, const char* where,
                                             int argument) {
    if (value.get_type() != sol::type::function) {
        throw std::invalid_argument(fmt::format("{}: argument {} must be a function (got {})", where,
                                                argument,
                                                sol::type_name(value.lua_state(), value.get_type())));
    }
    return value.as<sol::main_protected_function>();
}

}  // namespace vcmp_lua::bindings
