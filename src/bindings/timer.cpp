// The global thisTimer is set while a timer's callback runs.
#include <fmt/format.h>
#include <lua.hpp>
#include <sol/sol.hpp>

#include <cstdint>
#include <utility>
#include <vector>

#include "bindings/bindings.hpp"
#include "bindings/convert.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {
namespace {

// What scripts hold: the timer's id, never a pointer.
struct TimerHandle {
    std::uint64_t id;
};

}  // namespace

void RegisterTimer(sol::state& lua) {
    lua.new_usertype<TimerHandle>(
        "Timer", sol::no_constructor,

        // Timer.create(fn, interval_ms, repeats, ...): calls fn(...) every
        // interval_ms; repeats is -1 (until destroyed) or a positive count.
        "create",
        [](sol::this_state state, sol::main_object fn, sol::object interval, sol::object repeats,
           sol::variadic_args args) {
            lua_State* L = state;
            Scheduler& timers = Runtime::Require(L).Timers();
            std::vector<sol::main_object> extra;
            extra.reserve(args.size());
            for (int i = 0; i < static_cast<int>(args.size()); ++i) {
                extra.emplace_back(L, args.stack_index() + i);
            }
            const std::uint64_t id = timers.Create(
                RequireFunction(fn, "Timer.create", 1),
                ToInteger(interval, "Timer.create: argument 2 (interval)"),
                ToInteger(repeats, "Timer.create: argument 3 (repeat count)"), std::move(extra));
            sol::stack::push(L, TimerHandle{id});
            sol::main_object handle(L, -1);
            lua_pop(L, 1);
            timers.SetHandle(id, handle);
            return handle;
        },

        // Timer.destroy(timer) or timer:destroy(): false if it already ended.
        "destroy",
        [](sol::this_state L, const TimerHandle& timer) {
            return Runtime::Require(L).Timers().Destroy(timer.id);
        },

        // timer.active: false once it ended or was destroyed.
        "active", sol::property([](const TimerHandle& timer, sol::this_state L) {
            return Runtime::Require(L).Timers().Exists(timer.id);
        }),

        sol::meta_function::to_string,
        [](const TimerHandle& timer) { return fmt::format("Timer({})", timer.id); });
}

}  // namespace vcmp_lua::bindings
