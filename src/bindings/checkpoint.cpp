// A checkpoint stays until checkpoint:destroy() or the server deletes it,
// not until its handle is collected.
#include <sol/sol.hpp>

#include <cstdint>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kCheckpoint = EntityKind::Checkpoint;
using Self = Live<kCheckpoint>;

struct Rgba {
    std::int32_t r = 0;
    std::int32_t g = 0;
    std::int32_t b = 0;
    std::int32_t a = 255;
};

std::int32_t Channel(lua_State* L, int table, int i) {
    return static_cast<std::int32_t>(TableInteger(L, table, i, 0, 255));
}

// {r, g, b[, a]} at stack index `table`; a missing alpha stays `alpha`.
Rgba ReadColour(lua_State* L, int table, std::int32_t alpha) {
    if (lua_type(L, table) != LUA_TTABLE) {
        TypeError(L, table, "table");
    }
    Rgba colour{Channel(L, table, 1), Channel(L, table, 2), Channel(L, table, 3), alpha};
    if (TableLength(L, table) >= 4) {
        colour.a = Channel(L, table, 4);
    }
    return colour;
}

Rgba GetColour(const Self& self) {
    Rgba colour;
    Check(self.L,
          VCMP_FN(self, GetCheckPointColour)(self.id, &colour.r, &colour.g, &colour.b, &colour.a));
    return colour;
}

// Checkpoint.create(player, world, isSphere, x, y, z, {r, g, b[, a]}, radius)
// or Checkpoint.create(player, world, isSphere, {x, y, z}, {r, g, b[, a]},
// radius). player nil: visible to every player.
EntityRef<kCheckpoint> Create(lua_State* L, int first) {
    Runtime& runtime = Runtime::Require(L);
    const ArgReader args(L, first);
    std::int32_t owner = -1;
    if (!args.missing(1)) {
        owner = CheckLive<EntityKind::Player>(L, args.index(1)).id;
    }
    const auto world = args.Int<std::int32_t>(2);
    const bool sphere = args.Bool(3);
    int i = 4;
    const Vec3 position = args.Vector(i);
    const Rgba colour = ReadColour(L, args.index(i), 255);
    const float radius = args.Number(i + 1);
    const std::int32_t id = VCMP_LUA_API(runtime.api(), CreateCheckPoint)(
        owner, world, sphere ? 1 : 0, position.x, position.y, position.z, colour.r, colour.g,
        colour.b, colour.a, radius);
    return Created<kCheckpoint>(L, runtime, id);
}

}  // namespace

void RegisterCheckpoint(sol::state&, CheckpointType& type) {
    type["create"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type["new"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type[sol::call_constructor] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };

    type["destroy"] = [](Self self) {
        const bool deleted = Check(self.L, VCMP_FN(self, DeleteCheckPoint)(self.id));
        if (deleted) {
            self.pool().Release(self.id);
        }
        return deleted;
    };
    type["streamedForPlayer"] = [](Self self, Live<EntityKind::Player> player) {
        return VCMP_FN(self, IsCheckPointStreamedForPlayer)(self.id, player.id) != 0;
    };

    // The player it was created for, or nil for everyone.
    const auto owner = [](Self self) {
        return RefOf<EntityKind::Player>(*self.runtime, VCMP_FN(self, GetCheckPointOwner)(self.id));
    };
    const auto sphere = [](Self self) { return VCMP_FN(self, IsCheckPointSphere)(self.id) != 0; };
    type["getOwner"] = owner;
    type["owner"] = Property<kCheckpoint>(owner);
    type["isSphere"] = sphere;
    type["sphere"] = Property<kCheckpoint>(sphere);

    type["world"] =
        Property<kCheckpoint>([](Self self) { return VCMP_FN(self, GetCheckPointWorld)(self.id); },
                              [](Self self, Int32 world) {
                                  Check(self.L, VCMP_FN(self, SetCheckPointWorld)(self.id, world));
                              });
    type["radius"] = Property<kCheckpoint>(
        [](Self self) { return VCMP_FN(self, GetCheckPointRadius)(self.id); },
        [](Self self, Float radius) {
            Check(self.L, VCMP_FN(self, SetCheckPointRadius)(self.id, radius));
        });
    // checkpoint.color: {r, g, b, a}; assigning {r, g, b} keeps the alpha.
    type["color"] = Property<kCheckpoint>(
        [](Self self) {
            const Rgba c = GetColour(self);
            return sol::state_view(self.L).create_table_with(1, c.r, 2, c.g, 3, c.b, 4, c.a);
        },
        [](Self self, sol::object value) {
            value.push(self.L);
            const Rgba c = ReadColour(self.L, lua_gettop(self.L), GetColour(self).a);
            lua_pop(self.L, 1);
            Check(self.L, VCMP_FN(self, SetCheckPointColour)(self.id, c.r, c.g, c.b, c.a));
        });
    type["alpha"] = Property<kCheckpoint>(
        [](Self self) { return GetColour(self).a; },
        [](Self self, Int32 alpha) {
            const Rgba c = GetColour(self);
            Check(self.L, VCMP_FN(self, SetCheckPointColour)(self.id, c.r, c.g, c.b, alpha));
        });
    type["position"] = Property<kCheckpoint>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetCheckPointPosition)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 position) {
            Check(self.L, VCMP_FN(self, SetCheckPointPosition)(self.id, position.x, position.y,
                                                               position.z));
        });
}

}  // namespace vcmp_lua::bindings
