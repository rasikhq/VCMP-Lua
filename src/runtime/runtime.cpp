#include "runtime/runtime.hpp"

#include <lua.hpp>

#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "runtime/errors.hpp"
#include "runtime/log.hpp"
#include "runtime/preload.hpp"

namespace vcmp_lua {
namespace {

// The lua_CFunctions below run inside lua_pcall. A Lua error unwinds them
// with longjmp, so none of them may own a C++ object with a destructor.

// print(...): one line in the plugin log.
int Print(lua_State* L) {
    const int count = lua_gettop(L);
    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);
    for (int i = 1; i <= count; ++i) {
        if (i > 1) {
            luaL_addchar(&buffer, '\t');
        }
        luaL_tolstring(L, i, nullptr);
        luaL_addvalue(&buffer);
    }
    luaL_pushresult(&buffer);
    std::size_t length = 0;
    const char* text = lua_tolstring(L, -1, &length);
    log::Info("{}", std::string_view(text, length));
    return 0;
}

// Arg 1: light userdata, const Config*.
int SetupState(lua_State* L) {
    const auto* config = static_cast<const Config*>(lua_touserdata(L, 1));
    luaL_openlibs(L);
    RegisterBuiltins(L);

    lua_getglobal(L, LUA_LOADLIBNAME);
    lua_pushlstring(L, config->package_path.data(), config->package_path.size());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    lua_pushcfunction(L, &Print);
    lua_setglobal(L, "print");
    return 0;
}

// Arg 1: light userdata, NUL-terminated path. Returns the loaded chunk.
// Text mode only: crafted bytecode can crash Lua 5.4 (plan B6).
int LoadScriptFile(lua_State* L) {
    const auto* path = static_cast<const char*>(lua_touserdata(L, 1));
    if (luaL_loadfilex(L, path, "t") != LUA_OK) {
        return lua_error(L);
    }
    return 1;
}

}  // namespace

class Runtime::CallScope {
public:
    explicit CallScope(Runtime& runtime) noexcept : runtime_(runtime) { ++runtime_.call_depth_; }
    ~CallScope() { --runtime_.call_depth_; }

    CallScope(const CallScope&) = delete;
    CallScope& operator=(const CallScope&) = delete;

private:
    Runtime& runtime_;
};

Runtime::Runtime(Config config) : config_(std::move(config)) {
    lua_.emplace(&Panic);
    lua_State* L = lua_->lua_state();
    *static_cast<Runtime**>(lua_getextraspace(L)) = this;

    lua_pushcfunction(L, &SetupState);
    lua_pushlightuserdata(L, &config_);
    if (auto error = ProtectedCall(L, 1, 0)) {
        throw std::runtime_error("cannot set up the Lua state: " + *error);
    }
}

Runtime::~Runtime() {
    Shutdown();
}

bool Runtime::Usable() const noexcept {
    return lua_.has_value() && !closing_ && !dead_;
}

Runtime* Runtime::FromState(lua_State* L) noexcept {
    return *static_cast<Runtime**>(lua_getextraspace(L));
}

void Runtime::LoadScripts() {
    for (const std::string& script : config_.scripts) {
        if (!Usable()) {
            return;
        }
        lua_State* L = lua_->lua_state();
        CallScope scope(*this);
        log::Info("Loading {}", script);

        lua_pushcfunction(L, &LoadScriptFile);
        lua_pushlightuserdata(L, const_cast<char*>(script.c_str()));
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            log::Error("{}", ErrorText(L, -1));
            lua_pop(L, 1);
            continue;
        }
        if (auto error = ProtectedCall(L, 0, 0)) {
            log::Error("{}", *error);
        }
    }
}

void Runtime::Frame([[maybe_unused]] float elapsed_seconds) {
    if (!Usable()) {
        return;
    }
    // Phase 2: timers, deferred work and the HTTP/Copas pumps run here.
}

void Runtime::Shutdown() noexcept {
    if (!lua_.has_value()) {
        return;
    }
    // 1. From here on bindings raise "runtime shutting down".
    closing_ = true;

    // 2. Release every Lua reference C++ holds (handlers, timers, entity
    //    handles and data tables, pending HTTP callbacks). None exist yet.

    // 3. Close the Lua state: __gc and __close run while the subsystems are
    //    still alive. After a panic the state is inconsistent, so it is
    //    leaked instead.
    if (dead_) {
        [[maybe_unused]] auto* leaked = new (std::nothrow) sol::state(std::move(*lua_));
    }
    lua_.reset();

    // 4. Destroy the subsystems. None exist yet.
}

}  // namespace vcmp_lua
