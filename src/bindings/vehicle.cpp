// A vehicle stays until vehicle:destroy() or the server deletes it, not until
// its handle is collected.
#include <fmt/format.h>
#include <sol/sol.hpp>

#include <cstdint>
#include <tuple>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kVehicle = EntityKind::Vehicle;
constexpr EntityKind kPlayer = EntityKind::Player;
using Self = Live<kVehicle>;

// VehicleSpeed values.
enum Speed : std::int32_t { kNormal = 1, kNormalRelative = 2, kTurn = 3, kTurnRelative = 4 };

constexpr std::uint32_t kTaxiLight = 1u << 8;

// Vehicle.create(model, world, x, y, z, angle[, colour1[, colour2]]) or
// Vehicle.create(model, world, {x, y, z[, angle]}[, colour1[, colour2]]).
VehicleRef Create(lua_State* L, int first) {
    Runtime& runtime = Runtime::Require(L);
    const ArgReader args(L, first);
    const auto model = args.Int<std::int32_t>(1);
    const auto world = args.Int<std::int32_t>(2);
    int i = 3;
    float angle = 0;
    Vec3 position;
    if (args.type(3) == LUA_TTABLE) {
        position = args.Vector(i);
        const int angle_type = lua_rawgeti(L, args.index(3), 4);
        if (angle_type == LUA_TNUMBER) {
            angle = static_cast<float>(lua_tonumber(L, -1));
        } else if (angle_type != LUA_TNIL) {
            ArgError(L, args.index(3), fmt::format("angle (element 4) must be a number, got {}",
                                                   TypeName(L, -1)));
        }
        lua_pop(L, 1);
    } else {
        position = args.Vector(i);
        angle = args.Number(i++);
    }
    const auto primary = args.IntOr<std::int32_t>(i, -1);
    const auto secondary = args.IntOr<std::int32_t>(i + 1, -1);
    const std::int32_t id = VCMP_LUA_API(runtime.api(), CreateVehicle)(
        model, world, position.x, position.y, position.z, angle, primary, secondary);
    return Created<kVehicle>(L, runtime, id);
}

Vec3 GetSpeed(const Self& self, std::int32_t which) {
    Vec3 v;
    const auto get = VCMP_FN(self, GetVehicleSpeed);
    const auto get_turn = VCMP_FN(self, GetVehicleTurnSpeed);
    switch (which) {
        case kNormal:
            Check(self.L, get(self.id, &v.x, &v.y, &v.z, 0));
            break;
        case kNormalRelative:
            Check(self.L, get(self.id, &v.x, &v.y, &v.z, 1));
            break;
        case kTurn:
            Check(self.L, get_turn(self.id, &v.x, &v.y, &v.z, 0));
            break;
        case kTurnRelative:
            Check(self.L, get_turn(self.id, &v.x, &v.y, &v.z, 1));
            break;
        default:
            ArgError(self.L, 2, "VehicleSpeed value expected");
    }
    return v;
}

// vehicle:setSpeed([type,] x, y, z[, add]) or vehicle:setSpeed([type,] {x, y, z}[, add]).
bool SetSpeed(const Self& self, const ArgReader& args) {
    int i = 1;
    std::int32_t which = kNormal;
    // The type comes first if the argument after it starts the speed: a
    // table, or the first of three numbers followed by at most a boolean.
    const bool typed = args.type(1) == LUA_TNUMBER &&
                       (args.type(2) == LUA_TTABLE ||
                        (args.type(2) == LUA_TNUMBER && args.type(4) == LUA_TNUMBER));
    if (typed) {
        which = args.Int<std::int32_t>(i++);
    }
    const Vec3 v = args.Vector(i);
    const std::uint8_t add = args.BoolOr(i, false) ? 1 : 0;
    switch (which) {
        case kNormal:
        case kNormalRelative:
            return Check(self.L, VCMP_FN(self, SetVehicleSpeed)(self.id, v.x, v.y, v.z, add,
                                                                which == kNormalRelative ? 1 : 0));
        case kTurn:
        case kTurnRelative:
            return Check(self.L, VCMP_FN(self, SetVehicleTurnSpeed)(
                                     self.id, v.x, v.y, v.z, add, which == kTurnRelative ? 1 : 0));
        default:
            ArgError(self.L, args.index(1), "VehicleSpeed value expected");
    }
}

// {euler = {x, y, z}, quaternion = {x, y, z, w}}, as in 2.x.
sol::table GetRotation(const Self& self) {
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;
    sol::state_view lua(self.L);
    Check(self.L, VCMP_FN(self, GetVehicleRotationEuler)(self.id, &x, &y, &z));
    sol::table euler = lua.create_table_with(1, x, 2, y, 3, z);
    Check(self.L, VCMP_FN(self, GetVehicleRotation)(self.id, &x, &y, &z, &w));
    sol::table quaternion = lua.create_table_with(1, x, 2, y, 3, z, 4, w);
    return lua.create_table_with("euler", euler, "quaternion", quaternion);
}

// Sets a rotation from {x, y, z} (Euler), {x, y, z, w} (quaternion), or the
// table the rotation getter returns. The table is at stack index `index`.
void SetRotationFromTable(const Self& self, int index, bool spawn) {
    lua_State* L = self.L;
    index = lua_absindex(L, index);
    // The getter's own table: prefer its quaternion.
    for (const char* key : {"quaternion", "euler"}) {
        if (RawField(L, index, key) == LUA_TTABLE) {
            const int inner = lua_gettop(L);
            SetRotationFromTable(self, inner, spawn);
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);
    }
    const float x = TableNumber(L, index, 1);
    const float y = TableNumber(L, index, 2);
    const float z = TableNumber(L, index, 3);
    if (TableLength(L, index) >= 4) {
        const float w = TableNumber(L, index, 4);
        Check(L, spawn ? VCMP_FN(self, SetVehicleSpawnRotation)(self.id, x, y, z, w)
                       : VCMP_FN(self, SetVehicleRotation)(self.id, x, y, z, w));
    } else {
        Check(L, spawn ? VCMP_FN(self, SetVehicleSpawnRotationEuler)(self.id, x, y, z)
                       : VCMP_FN(self, SetVehicleRotationEuler)(self.id, x, y, z));
    }
}

void SetRotation(const Self& self, sol::object value) {
    if (value.get_type() != sol::type::table) {
        TypeError(self.L, 3, "table");
    }
    value.push(self.L);
    SetRotationFromTable(self, -1, false);
    lua_pop(self.L, 1);
}

}  // namespace

void RegisterVehicle(sol::state&, VehicleType& type) {
    type["create"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type["new"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    // Vehicle(...); sol2 drops the class table from the arguments.
    type[sol::call_constructor] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };

    type["resetAllHandlings"] = [](Ctx ctx) { VCMP_FN(ctx, ResetAllVehicleHandlings)(); };
    type["modelHandlingRuleExists"] = [](Ctx ctx, Int32 model, Int32 rule) {
        const bool exists = VCMP_FN(ctx, ExistsHandlingRule)(model, rule) != 0;
        CheckLast(ctx.L, ctx.api());
        return exists;
    };
    type["resetModelHandlingRule"] = [](Ctx ctx, Int32 model, Int32 rule) {
        return Check(ctx.L, VCMP_FN(ctx, ResetHandlingRule)(model, rule));
    };
    type["resetModelHandlingRules"] = [](Ctx ctx, Int32 model) {
        return Check(ctx.L, VCMP_FN(ctx, ResetHandling)(model));
    };
    type["getModelHandlingRule"] = [](Ctx ctx, Int32 model, Int32 rule) {
        const double value = VCMP_FN(ctx, GetHandlingRule)(model, rule);
        CheckLast(ctx.L, ctx.api());
        return value;
    };
    type["setModelHandlingRule"] = [](Ctx ctx, Int32 model, Int32 rule, Double value) {
        return Check(ctx.L, VCMP_FN(ctx, SetHandlingRule)(model, rule, value));
    };

    type["destroy"] = [](Self self) {
        const bool deleted = Check(self.L, VCMP_FN(self, DeleteVehicle)(self.id));
        if (deleted) {
            self.pool().Release(self.id);
        }
        return deleted;
    };
    type["respawn"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, RespawnVehicle)(self.id));
    };
    type["explode"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, ExplodeVehicle)(self.id));
    };
    // vehicle:repair() / vehicle:fix(): full health, no damage, lights fixed.
    const auto repair = [](Self self) {
        bool done = Check(self.L, VCMP_FN(self, SetVehicleHealth)(self.id, 1000.0f));
        done = Check(self.L, VCMP_FN(self, SetVehicleDamageData)(self.id, 0)) && done;
        const std::uint32_t lights = VCMP_FN(self, GetVehicleLightsData)(self.id);
        return Check(self.L, VCMP_FN(self, SetVehicleLightsData)(self.id, lights & 0xFFFFFF00u)) &&
               done;
    };
    type["repair"] = repair;
    type["fix"] = repair;

    type["streamedForPlayer"] = [](Self self, Live<kPlayer> player) {
        return VCMP_FN(self, IsVehicleStreamedForPlayer)(self.id, player.id) != 0;
    };
    type["getOption"] = [](Self self, Int32 option) {
        const bool on = VCMP_FN(self, GetVehicleOption)(
                            self.id, static_cast<vcmpVehicleOption>(option.value)) != 0;
        CheckLast(self.L, self.api());
        return on;
    };
    type["setOption"] = [](Self self, Int32 option, Boolean on) {
        return Check(self.L,
                     VCMP_FN(self, SetVehicleOption)(
                         self.id, static_cast<vcmpVehicleOption>(option.value), on ? 1 : 0));
    };
    type["getPartStatus"] = [](Self self, Int32 part) {
        const std::int32_t status = VCMP_FN(self, GetVehiclePartStatus)(self.id, part);
        CheckLast(self.L, self.api());
        return status;
    };
    type["setPartStatus"] = [](Self self, Int32 part, Int32 status) {
        return Check(self.L, VCMP_FN(self, SetVehiclePartStatus)(self.id, part, status));
    };
    type["getTyreStatus"] = [](Self self, Int32 tyre) {
        const std::int32_t status = VCMP_FN(self, GetVehicleTyreStatus)(self.id, tyre);
        CheckLast(self.L, self.api());
        return status;
    };
    type["setTyreStatus"] = [](Self self, Int32 tyre, Int32 status) {
        return Check(self.L, VCMP_FN(self, SetVehicleTyreStatus)(self.id, tyre, status));
    };

    // vehicle:getSpeed([VehicleSpeed.x]): {x, y, z}.
    type["getSpeed"] = [](Self self, Opt<Int32> which) {
        return GetSpeed(self, which.value_or(static_cast<std::int32_t>(kNormal)));
    };
    type["setSpeed"] = [](Self self, sol::variadic_args args) {
        return SetSpeed(self, ArgReader(self.L, args.stack_index()));
    };

    type["getRotation"] = [](Self self) { return GetRotation(self); };
    // vehicle:setRotation(table), (x, y, z) Euler, or (x, y, z, w) quaternion.
    type["setRotation"] = [](Self self, sol::variadic_args args) {
        const ArgReader reader(self.L, args.stack_index());
        if (reader.type(1) == LUA_TTABLE) {
            SetRotationFromTable(self, reader.index(1), false);
            return;
        }
        const float x = reader.Number(1);
        const float y = reader.Number(2);
        const float z = reader.Number(3);
        if (reader.missing(4)) {
            Check(self.L, VCMP_FN(self, SetVehicleRotationEuler)(self.id, x, y, z));
        } else {
            Check(self.L, VCMP_FN(self, SetVehicleRotation)(self.id, x, y, z, reader.Number(4)));
        }
    };

    type["resetHandling"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, ResetInstHandling)(self.id));
    };
    // vehicle:resetHandlingRule(rule) or vehicle:resetHandlingRule({rules}).
    type["resetHandlingRule"] = [](Self self, sol::variadic_args args) {
        const ArgReader reader(self.L, args.stack_index());
        if (reader.type(1) != LUA_TTABLE) {
            return Check(
                self.L, VCMP_FN(self, ResetInstHandlingRule)(self.id, reader.Int<std::int32_t>(1)));
        }
        bool all = true;
        const int table = reader.index(1);
        for (int i = 1, n = TableLength(self.L, table); i <= n; ++i) {
            const auto rule = static_cast<std::int32_t>(
                TableInteger(self.L, table, i, std::numeric_limits<std::int32_t>::min(),
                             std::numeric_limits<std::int32_t>::max()));
            all = Check(self.L, VCMP_FN(self, ResetInstHandlingRule)(self.id, rule)) && all;
        }
        return all;
    };
    type["hasHandlingRule"] = [](Self self, Int32 rule) {
        const bool exists = VCMP_FN(self, ExistsInstHandlingRule)(self.id, rule) != 0;
        CheckLast(self.L, self.api());
        return exists;
    };
    type["getHandlingRule"] = [](Self self, Int32 rule) {
        const double value = VCMP_FN(self, GetInstHandlingRule)(self.id, rule);
        CheckLast(self.L, self.api());
        return value;
    };
    type["setHandlingRule"] = [](Self self, Int32 rule, Double value) {
        return Check(self.L, VCMP_FN(self, SetInstHandlingRule)(self.id, rule, value));
    };

    type["get3DArrowToPlayer"] = [](Self self, Live<kPlayer> player) {
        return VCMP_FN(self, GetVehicle3DArrowForPlayer)(self.id, player.id) != 0;
    };
    type["set3DArrowToPlayer"] = [](Self self, Live<kPlayer> player, Boolean on) {
        return Check(self.L,
                     VCMP_FN(self, SetVehicle3DArrowForPlayer)(self.id, player.id, on ? 1 : 0));
    };

    const auto model = [](Self self) { return VCMP_FN(self, GetVehicleModel)(self.id); };
    type["getModel"] = model;
    type["model"] = Property<kVehicle>(model);
    // vehicle:getOccupant(slot): the player in the seat, or nil.
    type["getOccupant"] = [](Self self, Int32 slot) {
        const std::int32_t id = VCMP_FN(self, GetVehicleOccupant)(self.id, slot);
        const auto last_error = VCMP_LUA_FIND(self.api(), GetLastError);
        const vcmpError error = last_error != nullptr ? last_error() : vcmpErrorNone;
        if (error != vcmpErrorNoSuchEntity) {
            Check(self.L, error);
        }
        return error == vcmpErrorNone ? RefOf<kPlayer>(*self.runtime, id) : PlayerRef{};
    };
    // vehicle:getTurretRotation(): horizontal, vertical.
    type["getTurretRotation"] = [](Self self) {
        float horizontal = 0;
        float vertical = 0;
        Check(self.L, VCMP_FN(self, GetVehicleTurretRotation)(self.id, &horizontal, &vertical));
        return std::make_tuple(horizontal, vertical);
    };
    type["wrecked"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, IsVehicleWrecked)(self.id) != 0; });

    type["world"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, GetVehicleWorld)(self.id); },
                           [](Self self, Int32 world) {
                               Check(self.L, VCMP_FN(self, SetVehicleWorld)(self.id, world));
                           });
    type["health"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, GetVehicleHealth)(self.id); },
                           [](Self self, Float health) {
                               Check(self.L, VCMP_FN(self, SetVehicleHealth)(self.id, health));
                           });
    type["idleRespawnTime"] = Property<kVehicle>(
        [](Self self) { return VCMP_FN(self, GetVehicleIdleRespawnTimer)(self.id); },
        [](Self self, UInt32 ms) {
            Check(self.L, VCMP_FN(self, SetVehicleIdleRespawnTimer)(self.id, ms));
        });
    type["radio"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, GetVehicleRadio)(self.id); },
                           [](Self self, Int32 radio) {
                               Check(self.L, VCMP_FN(self, SetVehicleRadio)(self.id, radio));
                           });
    type["damage"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, GetVehicleDamageData)(self.id); },
                           [](Self self, UInt32 data) {
                               Check(self.L, VCMP_FN(self, SetVehicleDamageData)(self.id, data));
                           });
    type["immunity"] = Property<kVehicle>(
        [](Self self) { return VCMP_FN(self, GetVehicleImmunityFlags)(self.id); },
        [](Self self, UInt32 flags) {
            Check(self.L, VCMP_FN(self, SetVehicleImmunityFlags)(self.id, flags));
        });
    type["lightsData"] =
        Property<kVehicle>([](Self self) { return VCMP_FN(self, GetVehicleLightsData)(self.id); },
                           [](Self self, UInt32 data) {
                               Check(self.L, VCMP_FN(self, SetVehicleLightsData)(self.id, data));
                           });
    type["taxiLight"] = Property<kVehicle>(
        [](Self self) { return (VCMP_FN(self, GetVehicleLightsData)(self.id) & kTaxiLight) != 0; },
        [](Self self, Boolean on) {
            const std::uint32_t lights = VCMP_FN(self, GetVehicleLightsData)(self.id);
            Check(self.L, VCMP_FN(self, SetVehicleLightsData)(
                              self.id, on ? (lights | kTaxiLight) : (lights & ~kTaxiLight)));
        });

    // vehicle.color: {primary, secondary}; assigning a table with one
    // element keeps the other colour.
    type["color"] = Property<kVehicle>(
        [](Self self) {
            std::int32_t primary = 0;
            std::int32_t secondary = 0;
            Check(self.L, VCMP_FN(self, GetVehicleColour)(self.id, &primary, &secondary));
            return sol::state_view(self.L).create_table_with(1, primary, 2, secondary);
        },
        [](Self self, sol::object colours) {
            if (colours.get_type() != sol::type::table) {
                TypeError(self.L, 3, "table");
            }
            std::int32_t primary = 0;
            std::int32_t secondary = 0;
            Check(self.L, VCMP_FN(self, GetVehicleColour)(self.id, &primary, &secondary));
            colours.push(self.L);
            const int table = lua_gettop(self.L);
            primary = static_cast<std::int32_t>(
                TableInteger(self.L, table, 1, std::numeric_limits<std::int32_t>::min(),
                             std::numeric_limits<std::int32_t>::max(), primary));
            secondary = static_cast<std::int32_t>(
                TableInteger(self.L, table, 2, std::numeric_limits<std::int32_t>::min(),
                             std::numeric_limits<std::int32_t>::max(), secondary));
            lua_pop(self.L, 1);
            Check(self.L, VCMP_FN(self, SetVehicleColour)(self.id, primary, secondary));
        });

    type["position"] = Property<kVehicle>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetVehiclePosition)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 position) {
            Check(self.L, VCMP_FN(self, SetVehiclePosition)(self.id, position.x, position.y,
                                                            position.z, 0));
        });
    type["spawnPosition"] = Property<kVehicle>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetVehicleSpawnPosition)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 position) {
            Check(self.L, VCMP_FN(self, SetVehicleSpawnPosition)(self.id, position.x, position.y,
                                                                 position.z));
        });
    // vehicle.spawnRotation: Euler {x, y, z}; accepts {x, y, z, w} too.
    type["spawnRotation"] = Property<kVehicle>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetVehicleSpawnRotationEuler)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, sol::object rotation) {
            if (rotation.get_type() != sol::type::table) {
                TypeError(self.L, 3, "table");
            }
            rotation.push(self.L);
            SetRotationFromTable(self, -1, true);
            lua_pop(self.L, 1);
        });
    // vehicle.rotation (and vehicle.angle, as in 2.x): {euler = {x, y, z},
    // quaternion = {x, y, z, w}}; accepts that table, {x, y, z} or {x, y, z, w}.
    type["rotation"] =
        Property<kVehicle>([](Self self) { return GetRotation(self); },
                           [](Self self, sol::object value) { SetRotation(self, value); });
    type["angle"] =
        Property<kVehicle>([](Self self) { return GetRotation(self); },
                           [](Self self, sol::object value) { SetRotation(self, value); });
}

}  // namespace vcmp_lua::bindings
