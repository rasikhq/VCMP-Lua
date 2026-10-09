#include "core/scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace vcmp_lua {

Clock SteadyClock() {
    return [] {
        return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::steady_clock::now().time_since_epoch())
                                             .count());
    };
}

Scheduler::Scheduler(Invoker& invoker, Clock clock) : invoker_(invoker), clock_(std::move(clock)) {}

Scheduler::~Scheduler() = default;

std::uint64_t Scheduler::Create(sol::main_protected_function callback, std::int64_t interval_ms,
                                std::int64_t repeats, std::vector<sol::main_object> args) {
    if (interval_ms < 0 || interval_ms > kMaxInterval) {
        throw std::out_of_range("timer interval must be between 0 and 2^40 milliseconds");
    }
    if (repeats != kForever && repeats <= 0) {
        throw std::out_of_range("timer repeat count must be -1 (forever) or positive");
    }
    const std::uint64_t id = next_id_++;
    Timer& timer = timers_[id];
    timer.callback = std::move(callback);
    timer.args = std::move(args);
    timer.interval = interval_ms;
    timer.remaining = repeats;
    timer.due = clock_() + interval_ms;
    return id;
}

void Scheduler::SetHandle(std::uint64_t id, sol::main_object handle) {
    const auto it = timers_.find(id);
    if (it != timers_.end()) {
        it->second.handle = std::move(handle);
    }
}

bool Scheduler::Destroy(std::uint64_t id) {
    const auto it = timers_.find(id);
    if (it == timers_.end() || it->second.destroyed) {
        return false;
    }
    if (it->second.running) {
        it->second.destroyed = true;  // removed once its callback returns
    } else {
        timers_.erase(it);
    }
    return true;
}

bool Scheduler::Exists(std::uint64_t id) const {
    const auto it = timers_.find(id);
    return it != timers_.end() && !it->second.destroyed;
}

std::size_t Scheduler::Count() const noexcept {
    return static_cast<std::size_t>(
        std::count_if(timers_.begin(), timers_.end(),
                      [](const auto& entry) { return !entry.second.destroyed; }));
}

void Scheduler::Tick(lua_State* L) {
    if (timers_.empty()) {
        return;
    }
    // The whole tick counts as one call into Lua: it holds references to
    // timers between callbacks, so no shutdown may run.
    Invoker::Scope scope(invoker_);
    const std::int64_t now = clock_();
    due_.clear();
    for (const auto& [id, timer] : timers_) {
        if (!timer.destroyed && timer.due <= now) {
            due_.push_back(id);
        }
    }
    // Earliest first; ids (creation order) break ties.
    std::sort(due_.begin(), due_.end(), [this](std::uint64_t a, std::uint64_t b) {
        const std::int64_t due_a = timers_.at(a).due;
        const std::int64_t due_b = timers_.at(b).due;
        return due_a != due_b ? due_a < due_b : a < b;
    });
    // Taken out of the member while callbacks run; given back for reuse.
    std::vector<std::uint64_t> due;
    due.swap(due_);
    for (const std::uint64_t id : due) {
        const auto it = timers_.find(id);
        if (it == timers_.end() || it->second.destroyed) {
            continue;  // destroyed by an earlier callback
        }
        Run(L, id, it->second, now);
    }
    due_.swap(due);
}

void Scheduler::Run(lua_State* L, std::uint64_t id, Timer& timer, std::int64_t now) {
    if (timer.remaining > 0) {
        --timer.remaining;
    }
    // Fixed rate, but a timer that fell behind (a long frame) runs once and
    // then keeps its rate from now on, instead of catching up in a burst.
    timer.due += timer.interval;
    if (timer.due <= now) {
        timer.due = now + timer.interval;
    }

    // While the callback runs: timer.running is set (Destroy only marks the
    // timer), and thisTimer is the timer, as in v1. Both are undone also when
    // the call throws. thisTimer is set and restored with raw accesses, so no
    // metamethod of the globals table runs here.
    class RunningScope {
    public:
        RunningScope(const Invoker& invoker, lua_State* L, Timer& timer)
            : invoker_(invoker), L_(L), timer_(timer), top_(lua_gettop(L)) {
            if (!lua_checkstack(L, 4)) {
                throw std::runtime_error("Lua stack overflow");
            }
            lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
            lua_pushliteral(L, "thisTimer");
            lua_rawget(L, top_ + 1);  // the previous thisTimer, at top_ + 2
            lua_pushliteral(L, "thisTimer");
            timer.handle.push(L);
            lua_rawset(L, top_ + 1);
            timer_.running = true;
        }
        ~RunningScope() {
            timer_.running = false;
            if (invoker_.dead()) {
                return;
            }
            lua_settop(L_, top_ + 2);
            lua_pushliteral(L_, "thisTimer");
            lua_pushvalue(L_, top_ + 2);
            lua_rawset(L_, top_ + 1);
            lua_settop(L_, top_);
        }
        RunningScope(const RunningScope&) = delete;
        RunningScope& operator=(const RunningScope&) = delete;

    private:
        const Invoker& invoker_;
        lua_State* L_;
        Timer& timer_;
        int top_;
    };

    const auto push_args = [&timer](lua_State* thread) {
        const int count = static_cast<int>(timer.args.size());
        if (!lua_checkstack(thread, count)) {
            throw std::runtime_error("Lua stack overflow");
        }
        for (const sol::main_object& arg : timer.args) {
            arg.push(thread);
        }
        return count;
    };
    {
        RunningScope running(invoker_, L, timer);
        invoker_.Call(L, timer.callback, "Timer callback", push_args);
    }

    if (timer.destroyed || timer.remaining == 0) {
        timers_.erase(id);
    }
}

void Scheduler::Clear() noexcept {
    timers_.clear();
    due_.clear();
}

}  // namespace vcmp_lua
