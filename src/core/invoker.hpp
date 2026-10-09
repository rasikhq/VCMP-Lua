#pragma once

#include <lua.hpp>
#include <sol/sol.hpp>

#include <stdexcept>
#include <string_view>

#include "runtime/errors.hpp"
#include "runtime/log.hpp"

namespace vcmp_lua {

// The one way C++ calls a Lua function (plan B3.3): in protected mode, with a
// traceback, never after a panic, and counted, so that a shutdown or reload
// requested meanwhile can be deferred until the outermost call returns.
class Invoker {
public:
    explicit Invoker(lua_State* main) noexcept : main_(main) {}

    Invoker(const Invoker&) = delete;
    Invoker& operator=(const Invoker&) = delete;

    // The main thread of the Lua state.
    [[nodiscard]] lua_State* main() const noexcept { return main_; }

    // Number of calls into Lua that are active right now.
    [[nodiscard]] int depth() const noexcept { return depth_; }

    // True after a Lua panic: the state must not be touched again.
    [[nodiscard]] bool dead() const noexcept { return dead_; }
    void MarkDead() noexcept { dead_ = true; }

    // Counts one call into Lua for as long as it lives.
    class Scope {
    public:
        explicit Scope(Invoker& invoker) noexcept : invoker_(invoker) { ++invoker_.depth_; }
        ~Scope() { --invoker_.depth_; }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        Invoker& invoker_;
    };

    // Calls fn on thread L (the running thread) with the arguments push(L)
    // pushes; push returns how many. A Lua error is logged as "<what>: <error
    // and traceback>" and false is returned. The stack is left as it was.
    template <typename Push>
    bool Call(lua_State* L, const sol::reference& fn, std::string_view what, Push&& push) {
        if (dead_) {
            return false;
        }
        Scope scope(*this);
        StackRestore restore(*this, L);
        if (!lua_checkstack(L, 2)) {
            throw std::runtime_error("Lua stack overflow");
        }
        fn.push(L);
        const int nargs = push(L);
        if (auto error = ProtectedCall(L, nargs, 0)) {
            log::Error("{}: {}", what, *error);
            return false;
        }
        return true;
    }

    // Puts the stack top of L back where it was, also when a push throws;
    // but after a panic the state is left alone.
    class StackRestore {
    public:
        StackRestore(const Invoker& invoker, lua_State* L) noexcept
            : invoker_(invoker), L_(L), top_(lua_gettop(L)) {}
        ~StackRestore() {
            if (!invoker_.dead()) {
                lua_settop(L_, top_);
            }
        }
        StackRestore(const StackRestore&) = delete;
        StackRestore& operator=(const StackRestore&) = delete;

    private:
        const Invoker& invoker_;
        lua_State* L_;
        int top_;
    };

private:
    lua_State* main_;
    int depth_ = 0;
    bool dead_ = false;
};

}  // namespace vcmp_lua
