// The v1 globals v2 removed (plan B5). Each is a table that raises an error
// pointing to its replacement as soon as a script uses it, instead of the
// script failing later with "attempt to index a nil value".
#include <fmt/format.h>
#include <sol/sol.hpp>

#include <stdexcept>
#include <string>

#include "bindings/bindings.hpp"

namespace vcmp_lua::bindings {
namespace {

struct Removed {
    const char* name;
    const char* anchor;  // in docs/MIGRATION-v2.md
    const char* replaced;
};

constexpr Removed kRemoved[] = {
    {"MySQL", "mysql", "use require \"luasql.mysql\""},
    {"SQLite", "sqlite", "use require \"luasql.sqlite3\""},
    {"SqLite", "sqlite", "use require \"luasql.sqlite3\""},
    {"SQLiteDatabase", "sqlite", "use require \"luasql.sqlite3\""},
    {"Remote", "remote", "use require \"http\""},
    {"JSON", "json", "use require \"cjson\""},
    {"Thread", "thread", "Lua runs on the server's thread only"},
    {"dbg", "debugger", "it blocked the server waiting for console input"},
};

}  // namespace

void RegisterRemovedV1Stubs(sol::state& lua) {
    for (const Removed& removed : kRemoved) {
        const std::string message =
            fmt::format("{} was removed in v2: {} (see docs/MIGRATION-v2.md#{})", removed.name,
                        removed.replaced, removed.anchor);
        const auto fail = [message](sol::variadic_args) -> void {
            throw std::runtime_error(message);
        };
        sol::table metatable = lua.create_table();
        metatable[sol::meta_function::index] = fail;
        metatable[sol::meta_function::new_index] = fail;
        metatable[sol::meta_function::call] = fail;
        metatable["__metatable"] = false;
        sol::table stub = lua.create_named_table(removed.name);
        stub[sol::metatable_key] = metatable;
    }
}

}  // namespace vcmp_lua::bindings
