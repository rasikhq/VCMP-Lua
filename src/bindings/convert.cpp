#include "bindings/convert.hpp"

#include <fmt/format.h>
#include <lua.hpp>

#include <cmath>
#include <stdexcept>

namespace vcmp_lua::bindings {

std::int64_t ToInteger(const sol::object& value, const char* what) {
    lua_State* L = value.lua_state();
    const sol::type type = value.get_type();
    if (type != sol::type::number) {
        throw std::invalid_argument(
            fmt::format("{} must be an integer (got {})", what, sol::type_name(L, type)));
    }
    value.push(L);
    const bool is_integer = lua_isinteger(L, -1) != 0;
    const lua_Integer integer = lua_tointeger(L, -1);
    const lua_Number number = lua_tonumber(L, -1);
    lua_pop(L, 1);
    if (is_integer) {
        return integer;
    }
    // [-2^63, 2^63): where the conversion is defined.
    if (std::isfinite(number) && std::floor(number) == number &&
        number >= -9223372036854775808.0 && number < 9223372036854775808.0) {
        return static_cast<std::int64_t>(number);
    }
    throw std::invalid_argument(fmt::format("{} must be an integer (got {})", what, number));
}

}  // namespace vcmp_lua::bindings
