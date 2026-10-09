// Event: bind, unbind, cancel, create and trigger (v1 names and arguments).
#include <fmt/format.h>
#include <lua.hpp>
#include <sol/sol.hpp>

#include <stdexcept>
#include <string_view>

#include "bindings/bindings.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {
namespace {

std::size_t RequireEvent(const EventBus& events, std::string_view name) {
    const auto index = events.Find(name);
    if (!index) {
        throw std::invalid_argument(fmt::format("unknown event '{}'", name));
    }
    return *index;
}

}  // namespace

void RegisterEvent(sol::state& lua) {
    sol::table event = lua.create_named_table("Event");

    // Event.bind(name, fn): false if fn is bound to the event already.
    event["bind"] = [](sol::this_state L, std::string_view name, sol::main_object fn) {
        EventBus& events = Runtime::Require(L).Events();
        return events.Bind(L, RequireEvent(events, name), RequireFunction(fn, "Event.bind", 2));
    };

    // Event.unbind(name, fn): false if fn is not bound to the event.
    event["unbind"] = [](sol::this_state L, std::string_view name, sol::main_object fn) {
        EventBus& events = Runtime::Require(L).Events();
        return events.Unbind(L, RequireEvent(events, name),
                             RequireFunction(fn, "Event.unbind", 2));
    };

    // Event.cancel(): the handlers after the current one do not run, and a
    // cancellable server event is refused. Only inside an event handler.
    event["cancel"] = [](sol::this_state L) { Runtime::Require(L).Events().Cancel(); };

    // Event.create(name): a custom event. False if the name is taken.
    event["create"] = [](sol::this_state L, std::string_view name) {
        return Runtime::Require(L).Events().Create(name);
    };

    // Event.trigger(name, ...): calls the handlers with the arguments.
    // False when a handler cancelled the event.
    event["trigger"] = [](sol::this_state state, std::string_view name, sol::variadic_args args) {
        lua_State* L = state;
        EventBus& events = Runtime::Require(L).Events();
        const std::size_t index = RequireEvent(events, name);
        const int first = args.stack_index();
        const int count = static_cast<int>(args.size());
        const bool cancelled = events.Dispatch(L, index, [first, count](lua_State* thread) {
            if (!lua_checkstack(thread, count)) {
                throw std::runtime_error("Lua stack overflow");
            }
            for (int i = 0; i < count; ++i) {
                lua_pushvalue(thread, first + i);
            }
            return count;
        });
        return !cancelled;
    };
}

}  // namespace vcmp_lua::bindings
