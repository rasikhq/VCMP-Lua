#pragma once

#include <cstddef>
#include <cstdint>

namespace vcmp_lua {

// True when [offset, offset + size) lies within the first struct_size bytes.
constexpr bool FieldFits(std::uint32_t struct_size, std::size_t offset, std::size_t size) noexcept {
    return offset <= struct_size && size <= struct_size - offset;
}

}  // namespace vcmp_lua

// The server's PluginFuncs, PluginCallbacks and PluginInfo grew over time:
// fields were appended (e.g. OnPlayerModuleList). An older server passes a
// smaller struct and reports its size in structSize, so a field may be read or
// written only when it lies inside that size (plan B3.5).
#define VCMP_LUA_HAS_FIELD(ptr, Type, field) \
    ::vcmp_lua::FieldFits((ptr)->structSize, offsetof(Type, field), sizeof(Type::field))
