#pragma once

#include <deque>
#include <functional>

struct lua_State;

namespace vcmp_lua {

// The work that runs once per server frame, in the order it was added:
// timers now; the HTTP and Copas pumps in phase 4.
class FramePump {
public:
    using Step = std::function<void(lua_State*)>;

    void Add(Step step);

    // Runs every step on thread L (the main thread).
    void Run(lua_State* L);

    // Only while no step runs (Runtime::Shutdown).
    void Clear() noexcept { steps_.clear(); }

private:
    std::deque<Step> steps_;  // a step may Add(): no running step moves
};

}  // namespace vcmp_lua
