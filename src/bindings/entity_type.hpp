#pragma once

// The entity classes (Player, Vehicle, ...): RegisterEntities (entities.cpp)
// creates each usertype with the members every kind has, then hands it to
// the kind's own Register function to add the rest.

#include <sol/sol.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "bindings/args.hpp"
#include "core/entity_pool.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {

template <EntityKind K>
using EntityType = sol::usertype<EntityHandle<K>>;

using PlayerType = EntityType<EntityKind::Player>;
using VehicleType = EntityType<EntityKind::Vehicle>;
using ObjectType = EntityType<EntityKind::Object>;
using PickupType = EntityType<EntityKind::Pickup>;
using CheckpointType = EntityType<EntityKind::Checkpoint>;
using BlipType = EntityType<EntityKind::Blip>;
using BindType = EntityType<EntityKind::Bind>;

void RegisterPlayer(sol::state& lua, PlayerType& type);
void RegisterVehicle(sol::state& lua, VehicleType& type);
void RegisterObject(sol::state& lua, ObjectType& type);
void RegisterPickup(sol::state& lua, PickupType& type);
void RegisterCheckpoint(sol::state& lua, CheckpointType& type);
void RegisterBind(sol::state& lua, BindType& type);
void RegisterBlip(sol::state& lua, BlipType& type);

namespace detail {

template <typename F>
struct ValueArg : ValueArg<decltype(&F::operator())> {};
template <typename C, typename R, typename Self, typename Value>
struct ValueArg<R (C::*)(Self, Value) const> {
    using type = Value;
};

}  // namespace detail

// A property of kind K whose getter and setter take Live<K> as self, like
// the methods do. (sol2 passes a property's self only to a parameter of the
// usertype itself, so the handle is checked here.)
template <EntityKind K, typename Get>
auto Property(Get get) {
    return sol::property(
        [get](const EntityHandle<K>&, sol::this_state L) { return get(CheckLive<K>(L, 1)); });
}

template <EntityKind K, typename Get, typename Set>
auto Property(Get get, Set set) {
    using Value = typename detail::ValueArg<Set>::type;
    return sol::property(
        [get](const EntityHandle<K>&, sol::this_state L) { return get(CheckLive<K>(L, 1)); },
        [set](const EntityHandle<K>&, sol::this_state L, Value value) {
            set(CheckLive<K>(L, 1), std::move(value));
        });
}

// The handle of entity id, or nil when the pool has no live entity there
// (the server reports -1, or an id outside the pool, for "none").
template <EntityKind K>
EntityRef<K> RefOf(Runtime& runtime, std::int32_t id) {
    return runtime.Entities().Get(K).template Ref<K>(id);
}

// After a Create* call: the new entity's handle, marked as created by this
// runtime (a reload deletes it). id < 0 means the server refused; the error
// names the reason, e.g. "'create' failed: pool exhausted".
template <EntityKind K>
EntityRef<K> Created(lua_State* L, Runtime& runtime, std::int32_t id) {
    if (id < 0) {
        const auto last_error = VCMP_LUA_FIND(runtime.api(), GetLastError);
        const vcmpError error = last_error != nullptr ? last_error() : vcmpErrorNone;
        if (error != vcmpErrorNone) {
            Check(L, error);  // throws, except for vcmpErrorRequestDenied
        }
        throw std::runtime_error("'" + CurrentFunction(L) + "' failed: the server refused");
    }
    EntityPool& pool = runtime.Entities().Get(K);
    // The server reported the entity inside Create* already; adopting again
    // is a no-op (docs/internals.md).
    const EntityRef<K> ref = pool.template Seen<K>(id);
    pool.MarkCreatedByUs(id);
    return ref;
}

// Reads a string the server writes into a buffer (names, IPs, UIDs).
template <typename Fn, typename... Args>
std::string ReadText(lua_State* L, Fn fn, Args... args) {
    char buffer[256] = {};
    Check(L, fn(args..., buffer, sizeof(buffer)));
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
}

}  // namespace vcmp_lua::bindings
