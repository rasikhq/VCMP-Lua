// The members every entity class has. Every member checks that the entity
// still exists; the kinds' own files add the members that call the server.
#include <fmt/format.h>
#include <sol/sol.hpp>

#include <cstdint>
#include <string>

#include "bindings/bindings.hpp"
#include "bindings/entity_type.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {
namespace {

template <EntityKind K>
EntityPool& PoolOf(lua_State* L) {
    return Runtime::Require(L).Entities().Get(K);
}

template <EntityKind K>
bool IsAlive(lua_State* L, const EntityHandle<K>& self) {
    Runtime* runtime = Runtime::FromState(L);
    return runtime != nullptr && runtime->Usable() &&
           runtime->Entities().Get(K).Valid(self.id, self.generation);
}

template <EntityKind K>
EntityType<K> RegisterKind(sol::state& lua) {
    using Handle = EntityHandle<K>;
    EntityType<K> type = lua.new_usertype<Handle>(
        Traits(K).type_name, sol::no_constructor,

        "id", Property<K>([](Live<K> self) { return self.id; }), "getID",
        [](Live<K> self) { return self.id; },

        // False once the entity is gone; the only member that does not raise
        // for a dead handle (besides tostring).
        "valid",
        sol::property([](const Handle& self, sol::this_state L) { return IsAlive(L, self); }),

        // A table for scripts' own per-entity data, dropped with the entity.
        "data",
        sol::property(
            [](const Handle& self, sol::this_state L) {
                return PoolOf<K>(L).Data(L, self.id, self.generation);
            },
            [](const Handle& self, sol::this_state L, sol::main_table data) {
                PoolOf<K>(L).SetData(self.id, self.generation, std::move(data));
            }),

        // Kind.type() and handle:getType(): the class name.
        "type", [] { return Traits(K).type_name; }, "getType",
        [](const Handle&) { return Traits(K).type_name; },

        "findByID",
        [](Ctx ctx, Int32 id) { return ctx.runtime->Entities().Get(K).template Ref<K>(id); },

        "count", [](Ctx ctx) { return ctx.runtime->Entities().Get(K).AliveIds().size(); },

        // Kind.getActive(): {[id] = handle} of every entity of this kind.
        "getActive",
        [](Ctx ctx) {
            EntityPool& pool = ctx.runtime->Entities().Get(K);
            sol::table result = sol::state_view(ctx.L).create_table();
            for (const std::int32_t id : pool.AliveIds()) {
                result.raw_set(id, pool.template Ref<K>(id));
            }
            return result;
        },

        sol::meta_function::to_string,
        [](const Handle& self, sol::this_state L) {
            return fmt::format("{}({}{})", Traits(K).type_name, self.id,
                               IsAlive(L, self) ? "" : ", no longer exists");
        });
    return type;
}

}  // namespace

void RegisterEntities(sol::state& lua) {
    PlayerType player = RegisterKind<EntityKind::Player>(lua);
    RegisterPlayer(lua, player);
    VehicleType vehicle = RegisterKind<EntityKind::Vehicle>(lua);
    RegisterVehicle(lua, vehicle);
    ObjectType object = RegisterKind<EntityKind::Object>(lua);
    RegisterObject(lua, object);
    PickupType pickup = RegisterKind<EntityKind::Pickup>(lua);
    RegisterPickup(lua, pickup);
    CheckpointType checkpoint = RegisterKind<EntityKind::Checkpoint>(lua);
    RegisterCheckpoint(lua, checkpoint);
    BlipType blip = RegisterKind<EntityKind::Blip>(lua);
    RegisterBlip(lua, blip);
    BindType bind = RegisterKind<EntityKind::Bind>(lua);
    RegisterBind(lua, bind);
}

}  // namespace vcmp_lua::bindings
