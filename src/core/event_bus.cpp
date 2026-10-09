#include "core/event_bus.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace vcmp_lua {
namespace {

constexpr std::array<const char*, static_cast<std::size_t>(Event::Count)> kNames = {
#define VCMP_LUA_EVENT_NAME(id, name) name,
    VCMP_LUA_EVENTS(VCMP_LUA_EVENT_NAME)
#undef VCMP_LUA_EVENT_NAME
};

}  // namespace

EventBus::EventBus(Invoker& invoker) : invoker_(invoker) {
    for (const char* name : kNames) {
        Create(name);
    }
}

EventBus::~EventBus() = default;

const char* EventBus::Name(Event event) noexcept {
    return kNames[Index(event)];
}

std::optional<std::size_t> EventBus::Find(std::string_view name) const {
    const auto it = index_by_name_.find(std::string(name));
    if (it == index_by_name_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool EventBus::Create(std::string_view name) {
    std::string key(name);
    if (index_by_name_.contains(key)) {
        return false;
    }
    lists_.emplace_back();
    names_.push_back(key);
    index_by_name_.emplace(std::move(key), lists_.size() - 1);
    return true;
}

bool EventBus::Same(lua_State* L, const sol::reference& a, const sol::reference& b) {
    if (!lua_checkstack(L, 2)) {
        throw std::runtime_error("Lua stack overflow");
    }
    a.push(L);
    b.push(L);
    const bool same = lua_rawequal(L, -1, -2) != 0;
    lua_pop(L, 2);
    return same;
}

bool EventBus::Bind(lua_State* L, std::size_t event, sol::main_protected_function fn) {
    std::deque<Handler>& list = lists_.at(event);
    for (const Handler& handler : list) {
        if (handler.alive && Same(L, handler.fn, fn)) {
            return false;
        }
    }
    list.push_back(Handler{std::move(fn), true});
    return true;
}

bool EventBus::Unbind(lua_State* L, std::size_t event, const sol::reference& fn) {
    std::deque<Handler>& list = lists_.at(event);
    for (Handler& handler : list) {
        if (handler.alive && Same(L, handler.fn, fn)) {
            handler.alive = false;
            needs_compaction_ = true;
            if (cancelled_.empty()) {
                Compact();
            }
            return true;
        }
    }
    return false;
}

void EventBus::Cancel() {
    if (cancelled_.empty()) {
        throw std::logic_error("Event.cancel() called outside an event handler");
    }
    cancelled_.back() = true;
}

bool EventBus::HasHandlers(std::size_t event) const noexcept {
    return event < lists_.size() && !lists_[event].empty();
}

void EventBus::Clear() noexcept {
    for (std::deque<Handler>& list : lists_) {
        list.clear();
    }
    needs_compaction_ = false;
}

void EventBus::Compact() noexcept {
    for (std::deque<Handler>& list : lists_) {
        std::erase_if(list, [](const Handler& handler) { return !handler.alive; });
    }
    needs_compaction_ = false;
}

EventBus::DispatchFrame::DispatchFrame(EventBus& bus) : bus_(bus), index_(bus.cancelled_.size()) {
    bus_.cancelled_.push_back(false);
}

EventBus::DispatchFrame::~DispatchFrame() {
    if (!finished_) {
        bus_.cancelled_.resize(index_);
    }
}

bool EventBus::DispatchFrame::cancelled() const noexcept {
    return bus_.cancelled_[index_];
}

bool EventBus::DispatchFrame::Finish() {
    const bool was_cancelled = cancelled();
    bus_.cancelled_.resize(index_);
    finished_ = true;
    if (bus_.cancelled_.empty() && bus_.needs_compaction_) {
        bus_.Compact();
    }
    return was_cancelled;
}

}  // namespace vcmp_lua
