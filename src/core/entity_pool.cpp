#include "core/entity_pool.hpp"

#include <fmt/format.h>
#include <lua.hpp>

#include <stdexcept>
#include <utility>

#include "runtime/log.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua {
namespace {

// Pool sizes of the VC:MP 0.4 server (the same limits SqMod uses). Vehicle
// ids start at 1 (docs/internals.md), so that pool has one extra slot. The
// server has 50 key bind slots; 256 leaves room for a server with more.
constexpr std::array<EntityTraits, kEntityKindCount> kTraits = {{
    {"player", "Player", 0, 100},
    {"vehicle", "Vehicle", 1, 1001},
    {"object", "Object", 0, 3000},
    {"pickup", "Pickup", 0, 2000},
    {"checkpoint", "Checkpoint", 0, 2000},
    {"blip", "Blip", 0, 100},
    {"bind", "Bind", 0, 256},
}};

template <EntityKind K>
int MakeHandle(lua_State* L, std::int32_t id, std::uint32_t generation) {
    return sol::stack::push(L, EntityHandle<K>{id, generation});
}

using MakeHandleFn = int (*)(lua_State*, std::int32_t, std::uint32_t);

constexpr std::array<MakeHandleFn, kEntityKindCount> kMakeHandle = {
    &MakeHandle<EntityKind::Player>,     &MakeHandle<EntityKind::Vehicle>,
    &MakeHandle<EntityKind::Object>,     &MakeHandle<EntityKind::Pickup>,
    &MakeHandle<EntityKind::Checkpoint>, &MakeHandle<EntityKind::Blip>,
    &MakeHandle<EntityKind::Bind>,
};

}  // namespace

bool KeyBindExists(const ServerApi& api, std::int32_t id) {
    const auto bind_data = VCMP_LUA_FIND(api, GetKeyBindData);
    if (bind_data == nullptr) {
        return false;
    }
    std::uint8_t on_release = 0;
    std::int32_t keys[3] = {};
    return bind_data(id, &on_release, &keys[0], &keys[1], &keys[2]) == vcmpErrorNone &&
           (keys[0] != 0 || keys[1] != 0 || keys[2] != 0);
}

const EntityTraits& Traits(EntityKind kind) noexcept {
    return kTraits[static_cast<std::size_t>(kind)];
}

int PushEntity(lua_State* L, EntityKind kind, std::int32_t id, std::uint32_t generation) {
    Runtime* runtime = Runtime::FromState(L);
    if (runtime == nullptr || id < 0) {
        lua_pushnil(L);
        return 1;
    }
    return runtime->Entities().Get(kind).Push(L, id, generation);
}

// --- EntityPool --------------------------------------------------------------

EntityPool::EntityPool(EntityKind kind)
    : kind_(kind),
      traits_(Traits(kind)),
      slots_(static_cast<std::size_t>(traits_.capacity - traits_.first_id)) {}

EntityPool::~EntityPool() = default;

bool EntityPool::InRange(std::int32_t id) const noexcept {
    return id >= traits_.first_id && id < traits_.capacity;
}

EntityPool::SlotData& EntityPool::Slot(std::int32_t id) noexcept {
    return slots_[static_cast<std::size_t>(id - traits_.first_id)];
}

const EntityPool::SlotData& EntityPool::Slot(std::int32_t id) const noexcept {
    return slots_[static_cast<std::size_t>(id - traits_.first_id)];
}

bool EntityPool::Alive(std::int32_t id) const noexcept {
    return InRange(id) && Slot(id).alive;
}

bool EntityPool::Valid(std::int32_t id, std::uint32_t generation) const noexcept {
    return Alive(id) && Slot(id).generation == generation;
}

void EntityPool::Adopt(std::int32_t id) {
    if (!InRange(id)) {
        log::Warn("the server reported {} id {}, outside the pool [{}, {}); it is ignored",
                  traits_.name, id, traits_.first_id, traits_.capacity);
        return;
    }
    SlotData& slot = Slot(id);
    if (slot.alive) {
        return;
    }
    slot.alive = true;
    slot.created_by_us = false;
    ++slot.generation;
}

void EntityPool::Release(std::int32_t id) noexcept {
    if (!Alive(id)) {
        return;
    }
    SlotData& slot = Slot(id);
    slot.alive = false;
    slot.created_by_us = false;
    slot.handle.reset();
    slot.data.reset();
    slot.tag.clear();
}

void EntityPool::MarkCreatedByUs(std::int32_t id) noexcept {
    if (Alive(id)) {
        Slot(id).created_by_us = true;
    }
}

std::vector<std::int32_t> EntityPool::CreatedByUs() const {
    std::vector<std::int32_t> ids;
    for (std::int32_t id = traits_.first_id; id < traits_.capacity; ++id) {
        if (Slot(id).alive && Slot(id).created_by_us) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::int32_t> EntityPool::AliveIds() const {
    std::vector<std::int32_t> ids;
    for (std::int32_t id = traits_.first_id; id < traits_.capacity; ++id) {
        if (Slot(id).alive) {
            ids.push_back(id);
        }
    }
    return ids;
}

void EntityPool::Require(std::int32_t id, std::uint32_t generation) const {
    if (closed_) {
        throw std::runtime_error("runtime shutting down");
    }
    if (!Valid(id, generation)) {
        throw std::runtime_error(fmt::format("{} no longer exists", traits_.name));
    }
}

int EntityPool::Push(lua_State* L, std::int32_t id, std::uint32_t generation) {
    if (closed_ || !Valid(id, generation)) {
        lua_pushnil(L);
        return 1;
    }
    SlotData& slot = Slot(id);
    if (!slot.handle.valid()) {
        kMakeHandle[static_cast<std::size_t>(kind_)](L, id, generation);
        slot.handle = sol::main_object(L, -1);
        return 1;
    }
    return slot.handle.push(L);
}

sol::main_table EntityPool::Data(lua_State* L, std::int32_t id, std::uint32_t generation) {
    Require(id, generation);
    SlotData& slot = Slot(id);
    if (!slot.data.valid()) {
        slot.data = sol::main_table(L, sol::create);
    }
    return slot.data;
}

void EntityPool::SetData(std::int32_t id, std::uint32_t generation, sol::main_table data) {
    Require(id, generation);
    Slot(id).data = std::move(data);
}

const std::string& EntityPool::Tag(std::int32_t id, std::uint32_t generation) const {
    Require(id, generation);
    return Slot(id).tag;
}

void EntityPool::SetTag(std::int32_t id, std::uint32_t generation, std::string tag) {
    Require(id, generation);
    Slot(id).tag = std::move(tag);
}

void EntityPool::ReleaseRefs() noexcept {
    for (SlotData& slot : slots_) {
        slot.handle.reset();
        slot.data.reset();
    }
}

// --- EntityPools -------------------------------------------------------------

EntityPools::EntityPools()
    : pools_{{EntityPool(EntityKind::Player), EntityPool(EntityKind::Vehicle),
              EntityPool(EntityKind::Object), EntityPool(EntityKind::Pickup),
              EntityPool(EntityKind::Checkpoint), EntityPool(EntityKind::Blip),
              EntityPool(EntityKind::Bind)}} {}

EntityPool* EntityPools::FromServerPool(vcmpEntityPool pool) noexcept {
    switch (pool) {
        case vcmpEntityPoolVehicle:
            return &Get(EntityKind::Vehicle);
        case vcmpEntityPoolObject:
            return &Get(EntityKind::Object);
        case vcmpEntityPoolPickup:
            return &Get(EntityKind::Pickup);
        case vcmpEntityPoolCheckPoint:
            return &Get(EntityKind::Checkpoint);
        case vcmpEntityPoolBlip:
            return &Get(EntityKind::Blip);
        default:
            return nullptr;
    }
}

namespace {

constexpr std::array<std::pair<EntityKind, vcmpEntityPool>, 5> kServerPools = {{
    {EntityKind::Vehicle, vcmpEntityPoolVehicle},
    {EntityKind::Object, vcmpEntityPoolObject},
    {EntityKind::Pickup, vcmpEntityPoolPickup},
    {EntityKind::Checkpoint, vcmpEntityPoolCheckPoint},
    {EntityKind::Blip, vcmpEntityPoolBlip},
}};

}  // namespace

void EntityPools::Enumerate(const ServerApi& api) {
    const auto is_connected = VCMP_LUA_FIND(api, IsPlayerConnected);
    if (is_connected != nullptr) {
        EntityPool& players = Players();
        for (std::int32_t id = 0; id < Traits(EntityKind::Player).capacity; ++id) {
            if (is_connected(id) != 0) {
                players.Adopt(id);
            }
        }
    }
    if (VCMP_LUA_FIND(api, GetKeyBindData) != nullptr) {
        EntityPool& binds = Get(EntityKind::Bind);
        for (std::int32_t id = 0; id < Traits(EntityKind::Bind).capacity; ++id) {
            if (KeyBindExists(api, id)) {
                binds.Adopt(id);
            }
        }
    }
    const auto exists = VCMP_LUA_FIND(api, CheckEntityExists);
    if (exists == nullptr) {
        return;
    }
    for (const auto& [kind, server_pool] : kServerPools) {
        EntityPool& pool = Get(kind);
        const EntityTraits& traits = Traits(kind);
        for (std::int32_t id = traits.first_id; id < traits.capacity; ++id) {
            if (exists(server_pool, id) != 0) {
                pool.Adopt(id);
            }
        }
    }
}

void EntityPools::DeleteCreated(const ServerApi& api) {
    const auto delete_one = [&](EntityKind kind, std::int32_t id) {
        switch (kind) {
            case EntityKind::Vehicle:
                if (auto fn = VCMP_LUA_FIND(api, DeleteVehicle)) fn(id);
                break;
            case EntityKind::Object:
                if (auto fn = VCMP_LUA_FIND(api, DeleteObject)) fn(id);
                break;
            case EntityKind::Pickup:
                if (auto fn = VCMP_LUA_FIND(api, DeletePickup)) fn(id);
                break;
            case EntityKind::Checkpoint:
                if (auto fn = VCMP_LUA_FIND(api, DeleteCheckPoint)) fn(id);
                break;
            case EntityKind::Blip:
                if (auto fn = VCMP_LUA_FIND(api, DestroyCoordBlip)) fn(id);
                break;
            case EntityKind::Player:
            case EntityKind::Bind:
                break;
        }
    };
    for (const auto& [kind, server_pool] : kServerPools) {
        EntityPool& pool = Get(kind);
        for (const std::int32_t id : pool.CreatedByUs()) {
            delete_one(kind, id);  // the server reports the deletion, usually
            pool.Release(id);      // synchronously; releasing again is a no-op
        }
    }
    // Key binds are shared by every plugin: only ours are removed, never
    // with RemoveAllKeyBinds.
    EntityPool& binds = Get(EntityKind::Bind);
    const auto remove_bind = VCMP_LUA_FIND(api, RemoveKeyBind);
    for (const std::int32_t id : binds.CreatedByUs()) {
        if (remove_bind != nullptr) {
            remove_bind(id);
        }
        binds.Release(id);
    }
}

void EntityPools::Close() noexcept {
    for (EntityPool& pool : pools_) {
        pool.Close();
    }
}

void EntityPools::ReleaseRefs() noexcept {
    for (EntityPool& pool : pools_) {
        pool.ReleaseRefs();
    }
}

}  // namespace vcmp_lua
