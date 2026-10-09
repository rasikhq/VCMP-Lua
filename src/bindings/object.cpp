// Object: v1's members, ported with fixes:
// - The server owns objects: Object.create (also Object.new and
//   Object(...)) returns a handle; the object stays until object:destroy()
//   or the server deletes it (A2).
// - object.angle = {x, y, z} rotates to that angle; v1 rotated by it (A3).
// - rotateTo/rotateBy take a quaternion when the table has four elements;
//   v1 treated w = -1 as "no w".
#include <sol/sol.hpp>

#include <cstdint>
#include <limits>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kObject = EntityKind::Object;
using Self = Live<kObject>;

// Object.create(model, world, x, y, z[, alpha]) or
// Object.create(model, world, {x, y, z[, alpha]}).
EntityRef<kObject> Create(lua_State* L, int first) {
    Runtime& runtime = Runtime::Require(L);
    const ArgReader args(L, first);
    const auto model = args.Int<std::int32_t>(1);
    const auto world = args.Int<std::int32_t>(2);
    int i = 3;
    const bool table = args.type(3) == LUA_TTABLE;
    const Vec3 position = args.Vector(i);
    std::int32_t alpha = 255;
    if (table) {
        alpha = static_cast<std::int32_t>(TableInteger(L, args.index(3), 4, 0, 255, 255));
    } else {
        alpha = args.IntOr<std::uint8_t>(i, 255);
    }
    const std::int32_t id = VCMP_LUA_API(runtime.api(), CreateObject)(
        model, world, position.x, position.y, position.z, alpha);
    return Created<kObject>(L, runtime, id);
}

// moveTo/moveBy: (x, y, z[, ms]), ({x, y, z[, ms]}) or ({x, y, z}, ms).
template <typename Move>
bool MoveObject(const Self& self, const ArgReader& args, Move move) {
    int i = 1;
    const bool table = args.type(1) == LUA_TTABLE;
    const Vec3 to = args.Vector(i);
    std::uint32_t ms = 0;
    if (table && args.missing(2)) {
        ms = static_cast<std::uint32_t>(TableInteger(self.L, args.index(1), 4, 0,
                                                     std::numeric_limits<std::uint32_t>::max(), 0));
    } else {
        ms = args.IntOr<std::uint32_t>(i, 0);
    }
    return Check(self.L, move(self.id, to.x, to.y, to.z, ms));
}

// rotateTo/rotateBy: ({x, y, z}[, ms]) Euler or ({x, y, z, w}[, ms]) quaternion.
template <typename Quaternion, typename Euler>
bool RotateObject(const Self& self, const ArgReader& args, Quaternion quaternion, Euler euler) {
    if (args.type(1) != LUA_TTABLE) {
        TypeError(self.L, args.index(1), "table");
    }
    const int table = args.index(1);
    const float x = TableNumber(self.L, table, 1);
    const float y = TableNumber(self.L, table, 2);
    const float z = TableNumber(self.L, table, 3);
    const auto ms = args.IntOr<std::uint32_t>(2, 0);
    if (TableLength(self.L, table) >= 4) {
        return Check(self.L, quaternion(self.id, x, y, z, TableNumber(self.L, table, 4), ms));
    }
    return Check(self.L, euler(self.id, x, y, z, ms));
}

}  // namespace

void RegisterObject(sol::state&, ObjectType& type) {
    type["create"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type["new"] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };
    type[sol::call_constructor] = [](sol::this_state L, sol::variadic_args args) {
        return Create(L, args.stack_index());
    };

    // object:destroy(): deletes the object; the handle is dead afterwards.
    type["destroy"] = [](Self self) {
        const bool deleted = Check(self.L, VCMP_FN(self, DeleteObject)(self.id));
        self.pool().Release(self.id);
        return deleted;
    };
    type["streamedForPlayer"] = [](Self self, Live<EntityKind::Player> player) {
        return VCMP_FN(self, IsObjectStreamedForPlayer)(self.id, player.id) != 0;
    };

    const auto get_alpha = [](Self self) { return VCMP_FN(self, GetObjectAlpha)(self.id); };
    type["getAlpha"] = get_alpha;
    // object:setAlpha(alpha[, ms]): fades over ms milliseconds.
    type["setAlpha"] = [](Self self, Int32 alpha, Opt<UInt32> ms) {
        return Check(self.L, VCMP_FN(self, SetObjectAlpha)(self.id, alpha, ms.value_or(0u)));
    };
    type["alpha"] = Property<kObject>(get_alpha, [](Self self, Int32 alpha) {
        Check(self.L, VCMP_FN(self, SetObjectAlpha)(self.id, alpha, 0));
    });

    type["moveTo"] = [](Self self, sol::variadic_args args) {
        return MoveObject(self, ArgReader(self.L, args.stack_index()), VCMP_FN(self, MoveObjectTo));
    };
    type["moveBy"] = [](Self self, sol::variadic_args args) {
        return MoveObject(self, ArgReader(self.L, args.stack_index()), VCMP_FN(self, MoveObjectBy));
    };
    type["rotateTo"] = [](Self self, sol::variadic_args args) {
        return RotateObject(self, ArgReader(self.L, args.stack_index()),
                            VCMP_FN(self, RotateObjectTo), VCMP_FN(self, RotateObjectToEuler));
    };
    type["rotateBy"] = [](Self self, sol::variadic_args args) {
        return RotateObject(self, ArgReader(self.L, args.stack_index()),
                            VCMP_FN(self, RotateObjectBy), VCMP_FN(self, RotateObjectByEuler));
    };

    const auto model = [](Self self) { return VCMP_FN(self, GetObjectModel)(self.id); };
    type["getModel"] = model;
    type["model"] = Property<kObject>(model);

    type["world"] =
        Property<kObject>([](Self self) { return VCMP_FN(self, GetObjectWorld)(self.id); },
                          [](Self self, Int32 world) {
                              Check(self.L, VCMP_FN(self, SetObjectWorld)(self.id, world));
                          });
    // object.trackShots / trackTouch: whether onObjectShot / onObjectTouch fire.
    type["trackShots"] = Property<kObject>(
        [](Self self) { return VCMP_FN(self, IsObjectShotReportEnabled)(self.id) != 0; },
        [](Self self, Boolean on) {
            Check(self.L, VCMP_FN(self, SetObjectShotReportEnabled)(self.id, on ? 1 : 0));
        });
    type["trackTouch"] = Property<kObject>(
        [](Self self) { return VCMP_FN(self, IsObjectTouchedReportEnabled)(self.id) != 0; },
        [](Self self, Boolean on) {
            Check(self.L, VCMP_FN(self, SetObjectTouchedReportEnabled)(self.id, on ? 1 : 0));
        });
    type["position"] = Property<kObject>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetObjectPosition)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 position) {
            Check(self.L,
                  VCMP_FN(self, SetObjectPosition)(self.id, position.x, position.y, position.z));
        });
    // object.angle: Euler {x, y, z}; assigning rotates the object to it.
    type["angle"] = Property<kObject>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetObjectRotationEuler)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 angle) {
            Check(self.L,
                  VCMP_FN(self, RotateObjectToEuler)(self.id, angle.x, angle.y, angle.z, 0));
        });
}

}  // namespace vcmp_lua::bindings
