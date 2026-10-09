#pragma once

#include <sol/sol.hpp>

#include <cstdint>

namespace vcmp_lua::bindings {

// An integer, or a float with an integral value (Lua 5.4's `/` always gives
// a float: 1000 / 2 is 500.0). Anything else, or a value outside int64,
// throws "<what> must be an integer (got ...)".
std::int64_t ToInteger(const sol::object& value, const char* what);

}  // namespace vcmp_lua::bindings
