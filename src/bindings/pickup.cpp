// Pickup: v1's members. The server owns pickups: Pickup.create (also
// Pickup.new and Pickup(...)) returns a handle; the pickup stays until
// pickup:destroy() or the server deletes it (A2).
#include <sol/sol.hpp>

#include <cstdint>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kPickup = EntityKind::Pickup;
using Self = Live<kPickup>;

// Pickup.create(model, world, quantity, x, y, z[, alpha[, automatic]]) or
// Pickup.create(model, world, quantity, {x, y, z}[, alpha[, automatic]]).
// alpha defaults to 255 and automatic to true.
EntityRef<kPickup> Create(lua_State* L, int first) {
    Runtime& runtime = Runtime::Require(L);
    const ArgReader args(L, first);
    const auto model = args.Int<std::int32_t>(1);
    const auto world = args.Int<std::int32_t>(2);
    const auto quantity = args.Int<std::int32_t>(3);
    int i = 4;
    const Vec3 position = args.Vector(i);
    const auto alpha = args.IntOr<std::uint8_t>(i, 255);
    const bool automatic = args.BoolOr(i + 1, true);
    const std::int32_t id = VCMP_LUA_API(runtime.api(), CreatePickup)(
        model, world, quantity, position.x, position.y, position.z, alpha, automatic ? 1 : 0);
    return Created<kPickup>(L, runtime, id);
}

}  // namespace

void RegisterPickup(sol::state&, PickupType& type) {
    type["create"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type["new"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type[sol::call_constructor] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };

    // pickup:destroy(): deletes the pickup; the handle is dead afterwards.
    type["destroy"] = [](Self self) {
        const bool deleted = Check(self.L, VCMP_FN(self, DeletePickup)(self.id));
        if (deleted) {
            self.pool().Release(self.id);
        }
        return deleted;
    };
    type["respawn"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, RefreshPickup)(self.id));
    };
    type["streamedForPlayer"] = [](Self self, Live<EntityKind::Player> player) {
        return VCMP_FN(self, IsPickupStreamedForPlayer)(self.id, player.id) != 0;
    };
    type["getOption"] = [](Self self, Int32 option) {
        const bool on = VCMP_FN(self, GetPickupOption)(
                            self.id, static_cast<vcmpPickupOption>(option.value)) != 0;
        CheckLast(self.L, self.api());
        return on;
    };
    type["setOption"] = [](Self self, Int32 option, Boolean on) {
        return Check(self.L, VCMP_FN(self, SetPickupOption)(
                                 self.id, static_cast<vcmpPickupOption>(option.value), on ? 1 : 0));
    };

    const auto model = [](Self self) { return VCMP_FN(self, GetPickupModel)(self.id); };
    type["getModel"] = model;
    type["model"] = Property<kPickup>(model);
    type["quantity"] =
        Property<kPickup>([](Self self) { return VCMP_FN(self, GetPickupQuantity)(self.id); });

    type["world"] =
        Property<kPickup>([](Self self) { return VCMP_FN(self, GetPickupWorld)(self.id); },
                          [](Self self, Int32 world) {
                              Check(self.L, VCMP_FN(self, SetPickupWorld)(self.id, world));
                          });
    type["alpha"] =
        Property<kPickup>([](Self self) { return VCMP_FN(self, GetPickupAlpha)(self.id); },
                          [](Self self, Int32 alpha) {
                              Check(self.L, VCMP_FN(self, SetPickupAlpha)(self.id, alpha));
                          });
    type["auto"] = Property<kPickup>(
        [](Self self) { return VCMP_FN(self, IsPickupAutomatic)(self.id) != 0; },
        [](Self self, Boolean on) {
            Check(self.L, VCMP_FN(self, SetPickupIsAutomatic)(self.id, on ? 1 : 0));
        });
    type["autoTimer"] =
        Property<kPickup>([](Self self) { return VCMP_FN(self, GetPickupAutoTimer)(self.id); },
                          [](Self self, UInt32 ms) {
                              Check(self.L, VCMP_FN(self, SetPickupAutoTimer)(self.id, ms));
                          });
    type["position"] = Property<kPickup>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetPickupPosition)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 position) {
            Check(self.L,
                  VCMP_FN(self, SetPickupPosition)(self.id, position.x, position.y, position.z));
        });
}

}  // namespace vcmp_lua::bindings
