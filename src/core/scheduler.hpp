#pragma once

#include <sol/sol.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "core/invoker.hpp"

namespace vcmp_lua {

// Milliseconds on a monotonic clock. The default reads
// std::chrono::steady_clock; tests pass a fake one.
using Clock = std::function<std::int64_t()>;
Clock SteadyClock();

// Timers. Times are 64-bit milliseconds, so the clock never wraps, and Lua
// holds a timer by its id, never by a pointer.
//
// Tick() first collects the timers that are due, then calls them. A timer
// created during a tick waits for the next one. A timer destroyed while it
// runs is only marked, and removed once its callback returns.
class Scheduler {
public:
    static constexpr std::int64_t kForever = -1;
    // Longer intervals are refused: about 34 years.
    static constexpr std::int64_t kMaxInterval = std::int64_t{1} << 40;

    Scheduler(Invoker& invoker, Clock clock);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // A timer that calls callback(args...) every interval_ms milliseconds,
    // repeats times in all, or until destroyed if repeats is kForever.
    // Throws std::out_of_range for an interval outside [0, kMaxInterval] or a
    // repeat count that is neither kForever nor positive.
    std::uint64_t Create(sol::main_protected_function callback, std::int64_t interval_ms,
                         std::int64_t repeats, std::vector<sol::main_object> args);

    // The Lua value scripts hold for the timer; pushed as thisTimer while
    // its callback runs. Released with the timer.
    void SetHandle(std::uint64_t id, sol::main_object handle);

    // False if the timer does not exist (finished or destroyed).
    bool Destroy(std::uint64_t id);

    [[nodiscard]] bool Exists(std::uint64_t id) const;
    [[nodiscard]] std::size_t Count() const noexcept;

    // Runs the due timers on thread L (the main thread).
    void Tick(lua_State* L);

    // Releases every timer (Runtime::Shutdown, step 2).
    void Clear() noexcept;

private:
    struct Timer {
        sol::main_protected_function callback;
        std::vector<sol::main_object> args;
        sol::main_object handle;
        std::int64_t interval = 0;
        std::int64_t remaining = kForever;
        std::int64_t due = 0;
        bool running = false;
        bool destroyed = false;
    };

    void Run(lua_State* L, std::uint64_t id, Timer& timer, std::int64_t now);

    Invoker& invoker_;
    Clock clock_;
    std::map<std::uint64_t, Timer> timers_;  // std::map: nodes never move
    std::uint64_t next_id_ = 1;
    std::vector<std::uint64_t> due_;
};

}  // namespace vcmp_lua
