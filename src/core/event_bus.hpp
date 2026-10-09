#pragma once

#include <sol/sol.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/invoker.hpp"

namespace vcmp_lua {

// The built-in events, with their 2.x names.
#define VCMP_LUA_EVENTS(X)                                   \
    X(ServerInit, "onServerInit")                            \
    X(ServerShutdown, "onServerShutdown")                    \
    X(ServerFrame, "onServerFrame")                          \
    X(PluginCommand, "onPluginCommand")                      \
    X(ClientData, "onClientData")                            \
    X(PlayerModuleList, "onPlayerModuleList")                \
    X(PlayerConnection, "onPlayerConnection")                \
    X(PlayerConnect, "onPlayerConnect")                      \
    X(PlayerDisconnect, "onPlayerDisconnect")                \
    X(PlayerRequestClass, "onPlayerRequestClass")            \
    X(PlayerRequestSpawn, "onPlayerRequestSpawn")            \
    X(PlayerSpawn, "onPlayerSpawn")                          \
    X(PlayerWasted, "onPlayerWasted")                        \
    X(PlayerKill, "onPlayerKill")                            \
    X(PlayerUpdate, "onPlayerUpdate")                        \
    X(PlayerRequestEnterVehicle, "onPlayerRequestEnterVehicle") \
    X(PlayerEnterVehicle, "onPlayerEnterVehicle")            \
    X(PlayerExitVehicle, "onPlayerExitVehicle")              \
    X(PlayerNameChange, "onPlayerNameChange")                \
    X(PlayerStateChange, "onPlayerStateChange")              \
    X(PlayerActionChange, "onPlayerActionChange")            \
    X(PlayerFireChange, "onPlayerFireChange")                \
    X(PlayerCrouchChange, "onPlayerCrouchChange")            \
    X(PlayerGameKeysChange, "onPlayerGameKeysChange")        \
    X(PlayerBeginTyping, "onPlayerBeginTyping")              \
    X(PlayerFinishTyping, "onPlayerFinishTyping")            \
    X(PlayerAwayChange, "onPlayerAwayChange")                \
    X(PlayerMessage, "onPlayerMessage")                      \
    X(PlayerCommand, "onPlayerCommand")                      \
    X(PlayerPM, "onPlayerPM")                                \
    X(PlayerKeyDown, "onPlayerKeyDown")                      \
    X(PlayerKeyUp, "onPlayerKeyUp")                          \
    X(PlayerSpectate, "onPlayerSpectate")                    \
    X(PlayerCrashReport, "onPlayerCrashReport")              \
    X(VehicleUpdate, "onVehicleUpdate")                      \
    X(VehicleExplode, "onVehicleExplode")                    \
    X(VehicleRespawn, "onVehicleRespawn")                    \
    X(ObjectShot, "onObjectShot")                            \
    X(ObjectTouch, "onObjectTouch")                          \
    X(PickupPickAttempt, "onPickupPickAttempt")              \
    X(PickupPicked, "onPickupPicked")                        \
    X(PickupRespawn, "onPickupRespawn")                      \
    X(CheckpointEnter, "onCheckpointEnter")                  \
    X(CheckpointExit, "onCheckpointExit")                    \
    X(EntityPoolChange, "onEntityPoolChange")                \
    X(ServerPerformanceReport, "onServerPerformanceReport")  \
    X(EntityStreamingChange, "onEntityStreamingChange")

enum class Event : std::uint16_t {
#define VCMP_LUA_EVENT_ENUM(id, name) id,
    VCMP_LUA_EVENTS(VCMP_LUA_EVENT_ENUM)
#undef VCMP_LUA_EVENT_ENUM
        Count
};

// Handlers per event, in bind order.
//
// - Events are indexed: the built-ins by their enum value, custom events
//   (Event.create) after them. Names are looked up only by the Lua API.
// - A dispatch calls the handlers that existed when it started, by index.
//   Handlers live in a std::deque, so binding during a dispatch moves none.
// - Unbinding marks the handler dead. Dead handlers are removed only when no
//   dispatch of any event is active.
// - Each dispatch has its own cancel flag; Event.cancel() sets the innermost.
class EventBus {
public:
    explicit EventBus(Invoker& invoker);
    ~EventBus();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    static std::size_t Index(Event event) noexcept { return static_cast<std::size_t>(event); }
    static const char* Name(Event event) noexcept;

    // The index of the built-in or custom event called name.
    [[nodiscard]] std::optional<std::size_t> Find(std::string_view name) const;

    // Adds a custom event. False if an event with that name exists.
    bool Create(std::string_view name);

    // False if fn is already bound to the event. L: the running thread.
    bool Bind(lua_State* L, std::size_t event, sol::main_protected_function fn);

    // False if fn is not bound to the event. L: the running thread.
    bool Unbind(lua_State* L, std::size_t event, const sol::reference& fn);

    // Cancels the innermost active dispatch: its remaining handlers do not
    // run. Throws when no dispatch is active.
    void Cancel();

    [[nodiscard]] bool HasHandlers(std::size_t event) const noexcept;

    // Calls the event's handlers on thread L with the arguments push(L)
    // pushes (push returns how many; it runs once per handler). A handler
    // that raises an error is logged and the next one runs. Returns true when
    // a handler cancelled the event.
    template <typename Push>
    bool Dispatch(lua_State* L, std::size_t event, Push&& push) {
        if (!HasHandlers(event)) {
            return false;
        }
        // The whole dispatch counts as one call into Lua: between handlers it
        // holds references into the handler lists, so no shutdown may run.
        Invoker::Scope scope(invoker_);
        DispatchFrame frame(*this);
        const std::size_t count = lists_[event].size();
        for (std::size_t i = 0; i < count; ++i) {
            const Handler& handler = lists_[event][i];
            if (!handler.alive) {
                continue;
            }
            invoker_.Call(L, handler.fn, names_[event], push);
            if (frame.cancelled()) {
                break;
            }
        }
        return frame.Finish();
    }

    // Dispatch with C++ values as arguments (pushed with sol2).
    template <typename... Args>
    bool Emit(lua_State* L, Event event, const Args&... args) {
        return Dispatch(L, Index(event), [&]([[maybe_unused]] lua_State* thread) {
            int count = 0;
            ((count += sol::stack::push(thread, args)), ...);
            return count;
        });
    }

    // Releases every handler (Runtime::Shutdown, step 2).
    void Clear() noexcept;

private:
    struct Handler {
        sol::main_protected_function fn;
        bool alive = true;
    };

    // One active dispatch: pushed on construction, popped by Finish() (or by
    // the destructor if a handler call threw).
    class DispatchFrame {
    public:
        explicit DispatchFrame(EventBus& bus);
        ~DispatchFrame();
        DispatchFrame(const DispatchFrame&) = delete;
        DispatchFrame& operator=(const DispatchFrame&) = delete;

        [[nodiscard]] bool cancelled() const noexcept;
        // Pops the frame, compacts if it was the outermost; returns cancelled().
        bool Finish();

    private:
        EventBus& bus_;
        std::size_t index_;
        bool finished_ = false;
    };

    static bool Same(lua_State* L, const sol::reference& a, const sol::reference& b);
    void Compact() noexcept;

    Invoker& invoker_;
    // Deques: adding an event moves no list and no name, so a dispatch can
    // keep references to both across handler calls.
    std::deque<std::deque<Handler>> lists_;
    std::deque<std::string> names_;
    std::unordered_map<std::string, std::size_t> index_by_name_;
    std::vector<bool> cancelled_;  // one per active dispatch, innermost last
    bool needs_compaction_ = false;
};

}  // namespace vcmp_lua
