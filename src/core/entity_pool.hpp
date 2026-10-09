#pragma once

#include <vcmp.h>

#include <sol/sol.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "plugin/api_guard.hpp"

namespace vcmp_lua {

// The server entities scripts get handles to. Key binds are not in the
// server's entity pools (no pool events), but are held the same way.
enum class EntityKind : std::uint8_t { Player, Vehicle, Object, Pickup, Checkpoint, Blip, Bind };
inline constexpr std::size_t kEntityKindCount = 7;

struct EntityTraits {
    const char* name;       // in error messages: "vehicle no longer exists"
    const char* type_name;  // the Lua class: "Vehicle"
    std::int32_t first_id;  // the server's first id for this kind
    std::int32_t capacity;  // valid ids are [first_id, capacity)
};

// Whether key bind slot id holds a bind. The 0.4 server answers
// GetKeyBindData for every one of its 50 slots, with keys 0, 0, 0 for an
// unused one (docs/internals.md).
bool KeyBindExists(const ServerApi& api, std::int32_t id);

// Pool limits of the VC:MP 0.4 server. CheckEntityExists never reports an
// index out of bounds (docs/internals.md), so these bound the enumeration.
const EntityTraits& Traits(EntityKind kind) noexcept;

// The userdata scripts hold. Only EntityPool creates it, once per entity
// lifetime, so the same entity is always the same Lua value: == and table
// keys work without an __eq of our own.
template <EntityKind K>
struct EntityHandle {
    std::int32_t id;
    std::uint32_t generation;
};

// What C++ passes to Lua for an entity. It is pushed only through
// sol_lua_push below, as the cached handle of a live entity, else nil.
template <EntityKind K>
struct EntityRef {
    std::int32_t id = -1;
    std::uint32_t generation = 0;
};

using PlayerRef = EntityRef<EntityKind::Player>;
using VehicleRef = EntityRef<EntityKind::Vehicle>;
using BindRef = EntityRef<EntityKind::Bind>;

// Pushes the handle of entity (kind, id, generation) of the runtime that owns
// L, or nil when it no longer exists, id is -1, or the runtime is closing.
int PushEntity(lua_State* L, EntityKind kind, std::int32_t id, std::uint32_t generation);

template <EntityKind K>
int sol_lua_push(sol::types<EntityRef<K>>, lua_State* L, const EntityRef<K>& ref) {
    return PushEntity(L, K, ref.id, ref.generation);
}

// One kind's slots. Lua never owns an entity: the server does. A slot is
// adopted when the entity appears and released when it goes, and both are
// idempotent, because the server reports pool changes synchronously inside
// Create*/Delete* while the binding that called it still runs. The
// generation changes once per lifetime, so a handle to a dead entity never
// matches a newer one that reuses its id.
class EntityPool {
public:
    explicit EntityPool(EntityKind kind);
    ~EntityPool();

    EntityPool(const EntityPool&) = delete;
    EntityPool& operator=(const EntityPool&) = delete;

    [[nodiscard]] EntityKind kind() const noexcept { return kind_; }
    [[nodiscard]] bool InRange(std::int32_t id) const noexcept;
    [[nodiscard]] bool Alive(std::int32_t id) const noexcept;
    [[nodiscard]] bool Valid(std::int32_t id, std::uint32_t generation) const noexcept;

    // Starts a lifetime unless the slot is alive already. Ids outside the
    // pool are ignored with a warning. Touches no Lua state.
    void Adopt(std::int32_t id);

    // Ends the lifetime, if any, and drops the handle and data table.
    void Release(std::int32_t id) noexcept;

    // Adopt, then the reference (invalid for an id outside the pool).
    template <EntityKind K>
    EntityRef<K> Seen(std::int32_t id) {
        Adopt(id);
        return Ref<K>(id);
    }

    // The reference to the current lifetime; invalid if not alive.
    template <EntityKind K>
    EntityRef<K> Ref(std::int32_t id) const noexcept {
        if (!Alive(id)) {
            return {};
        }
        return {id, Slot(id).generation};
    }

    // Entities this runtime created are deleted when it reloads.
    void MarkCreatedByUs(std::int32_t id) noexcept;
    [[nodiscard]] std::vector<std::int32_t> CreatedByUs() const;
    [[nodiscard]] std::vector<std::int32_t> AliveIds() const;

    // Throws "<kind> no longer exists" unless (id, generation) is alive.
    void Require(std::int32_t id, std::uint32_t generation) const;

    // The cached handle (created on first use), or nil.
    int Push(lua_State* L, std::int32_t id, std::uint32_t generation);

    // The entity's data table (created on first use). Require()s the entity.
    sol::main_table Data(lua_State* L, std::int32_t id, std::uint32_t generation);
    void SetData(std::int32_t id, std::uint32_t generation, sol::main_table data);

    // A script-chosen name (key binds' tag), dropped with the entity.
    // Require()s the entity.
    [[nodiscard]] const std::string& Tag(std::int32_t id, std::uint32_t generation) const;
    void SetTag(std::int32_t id, std::uint32_t generation, std::string tag);

    // Runtime::Shutdown: no new handles from step 1 on, and every Lua
    // reference released in step 2.
    void Close() noexcept { closed_ = true; }
    void ReleaseRefs() noexcept;

private:
    struct SlotData {
        bool alive = false;
        bool created_by_us = false;
        std::uint32_t generation = 0;
        sol::main_object handle;
        sol::main_table data;
        std::string tag;
    };

    SlotData& Slot(std::int32_t id) noexcept;
    const SlotData& Slot(std::int32_t id) const noexcept;

    EntityKind kind_;
    const EntityTraits& traits_;
    std::vector<SlotData> slots_;  // index = id - first_id
    bool closed_ = false;
};

// One pool per kind, with the server-facing operations on all of them.
class EntityPools {
public:
    EntityPools();

    EntityPool& Get(EntityKind kind) noexcept { return pools_[static_cast<std::size_t>(kind)]; }
    EntityPool& Players() noexcept { return Get(EntityKind::Player); }

    // The pool behind a vcmpEntityPool, or null (radio streams).
    EntityPool* FromServerPool(vcmpEntityPool pool) noexcept;

    // Adopts every player, entity and key bind that exists already (scripts
    // load after other plugins may have created some, and again on reload).
    void Enumerate(const ServerApi& api);

    // Deletes the entities and key binds this runtime created (on reload).
    void DeleteCreated(const ServerApi& api);

    void Close() noexcept;
    void ReleaseRefs() noexcept;

private:
    std::array<EntityPool, kEntityKindCount> pools_;
};

}  // namespace vcmp_lua
