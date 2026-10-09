#include "core/frame_pump.hpp"

#include <utility>

namespace vcmp_lua {

void FramePump::Add(Step step) {
    steps_.push_back(std::move(step));
}

void FramePump::Run(lua_State* L) {
    // By index up to the current size: a step added by a step runs in the
    // same frame, and std::deque moves no existing step.
    for (std::size_t i = 0; i < steps_.size(); ++i) {
        steps_[i](L);
    }
}

}  // namespace vcmp_lua
