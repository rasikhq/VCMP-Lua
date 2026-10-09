// Key binds are shared by every plugin and raise no pool events, so a handle
// is checked against the server (GetKeyBindData) on every use; a slot whose
// keys are all 0 is free (docs/internals.md). A bind stays until
// bind:destroy(), not until its handle is collected.
#include <sol/sol.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kBind = EntityKind::Bind;
using Self = Live<kBind>;

struct BindData {
    std::uint8_t on_release = 0;
    std::int32_t keys[3] = {};
};

bool ReadBind(const ServerApi& api, std::int32_t id, BindData& data) {
    return VCMP_LUA_API(api, GetKeyBindData)(id, &data.on_release, &data.keys[0], &data.keys[1],
                                             &data.keys[2]) == vcmpErrorNone;
}

// Bind.create(signalOnRelease, key1[, key2[, key3]]).
BindRef Create(Ctx ctx, Boolean on_release, Int32 key1, Opt<Int32> key2, Opt<Int32> key3) {
    const std::int32_t slot = VCMP_FN(ctx, GetKeyBindUnusedSlot)();
    if (slot < 0) {
        throw std::runtime_error("'" + CurrentFunction(ctx.L) + "' failed: no free key bind slot");
    }
    if (!Check(ctx.L, VCMP_FN(ctx, RegisterKeyBind)(slot, on_release ? 1 : 0, key1,
                                                    key2.value_or(0), key3.value_or(0)))) {
        throw std::runtime_error("'" + CurrentFunction(ctx.L) + "' failed: request denied");
    }
    return Created<kBind>(ctx.L, *ctx.runtime, slot);
}

// False if the server refused; a bind already gone counts as removed.
bool Remove(const Self& self) {
    const vcmpError error = VCMP_FN(self, RemoveKeyBind)(self.id);
    if (error == vcmpErrorNone || error == vcmpErrorNoSuchEntity) {
        self.pool().Release(self.id);
        return true;
    }
    return Check(self.L, error);
}

}  // namespace

void RequireBindExists(Runtime& runtime, std::int32_t id) {
    if (!KeyBindExists(runtime.api(), id)) {
        runtime.Entities().Get(kBind).Release(id);
        throw std::runtime_error("bind no longer exists");
    }
}

void RegisterBind(sol::state&, BindType& type) {
    type["create"] = &Create;
    type["new"] = &Create;
    type[sol::call_constructor] = &Create;

    // Bind.findByTag(tag): the first bind with that tag, or nil.
    type["findByTag"] = [](Ctx ctx, String tag) {
        EntityPool& pool = ctx.runtime->Entities().Get(kBind);
        for (const std::int32_t id : pool.AliveIds()) {
            const BindRef ref = pool.Ref<kBind>(id);
            if (pool.Tag(ref.id, ref.generation) == tag.value) {
                return ref;
            }
        }
        return BindRef{};
    };

    // Removes only the binds this plugin created, not other plugins' binds.
    type["clearAllBinds"] = [](Ctx ctx) {
        EntityPool& pool = ctx.runtime->Entities().Get(kBind);
        for (const std::int32_t id : pool.CreatedByUs()) {
            const BindRef ref = pool.Ref<kBind>(id);
            Remove(Self{ctx.runtime, ctx.L, ref.id, ref.generation});
        }
    };

    type["destroy"] = [](Self self) { return Remove(self); };

    type["getData"] = [](Self self) {
        BindData data;
        if (!ReadBind(self.api(), self.id, data)) {
            throw std::runtime_error("bind no longer exists");
        }
        return sol::state_view(self.L).create_table_with("keyOne", data.keys[0], "keyTwo",
                                                         data.keys[1], "keyThree", data.keys[2],
                                                         "signalsOnRelease", data.on_release != 0);
    };

    // bind.tag: a name of the script's choosing, for Bind.findByTag.
    type["tag"] = Property<kBind>(
        [](Self self) { return self.pool().Tag(self.id, self.generation); },
        [](Self self, String tag) { self.pool().SetTag(self.id, self.generation, tag.value); });
}

}  // namespace vcmp_lua::bindings
