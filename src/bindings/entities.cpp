// The entity classes (Player, Vehicle, ...): handles whose every member
// checks that the entity still exists (plan B4). Phase 3 adds the members
// that call the server.
#include <fmt/format.h>
#include <sol/sol.hpp>

#include <cstdint>
#include <string>

#include "bindings/bindings.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {
namespace {

template <EntityKind K>
EntityPool& PoolOf(lua_State* L) {
    return Runtime::Require(L).Entities().Get(K);
}

template <EntityKind K>
void RegisterKind(sol::state& lua) {
    using Handle = EntityHandle<K>;
    lua.new_usertype<Handle>(
        Traits(K).type_name, sol::no_constructor,

        "id", sol::property([](const Handle& self, sol::this_state L) {
            PoolOf<K>(L).Require(self.id, self.generation);
            return self.id;
        }),

        // A table for scripts' own per-entity data, dropped with the entity.
        "data",
        sol::property(
            [](const Handle& self, sol::this_state L) {
                return PoolOf<K>(L).Data(L, self.id, self.generation);
            },
            [](const Handle& self, sol::this_state L, sol::main_table data) {
                PoolOf<K>(L).SetData(self.id, self.generation, std::move(data));
            }),

        sol::meta_function::to_string, [](const Handle& self, sol::this_state L) {
            Runtime* runtime = Runtime::FromState(L);
            const bool alive = runtime != nullptr && runtime->Usable() &&
                               runtime->Entities().Get(K).Valid(self.id, self.generation);
            return fmt::format("{}({}{})", Traits(K).type_name, self.id,
                               alive ? "" : ", no longer exists");
        });
}

}  // namespace

void RegisterEntities(sol::state& lua) {
    RegisterKind<EntityKind::Player>(lua);
    RegisterKind<EntityKind::Vehicle>(lua);
    RegisterKind<EntityKind::Object>(lua);
    RegisterKind<EntityKind::Pickup>(lua);
    RegisterKind<EntityKind::Checkpoint>(lua);
    RegisterKind<EntityKind::Blip>(lua);
}

}  // namespace vcmp_lua::bindings
