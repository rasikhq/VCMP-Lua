#include "bindings/bindings.hpp"

#include <fmt/format.h>
#include <sol/sol.hpp>

#include <stdexcept>

namespace vcmp_lua::bindings {

void Register(sol::state& lua) {
    RegisterEvent(lua);
    RegisterTimer(lua);
    RegisterEntities(lua);
    RegisterServer(lua);
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
