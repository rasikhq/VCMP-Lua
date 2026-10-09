#pragma once

#include <vcmp.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace vcmp_lua {

// True when [offset, offset + size) lies within the first struct_size bytes.
constexpr bool FieldFits(std::uint32_t struct_size, std::size_t offset, std::size_t size) noexcept {
    return offset <= struct_size && size <= struct_size - offset;
}

// Raised by bindings whose server function this server version lacks.
class NotSupported : public std::runtime_error {
public:
    explicit NotSupported(const char* function)
        : std::runtime_error(std::string(function) + " is not supported by this server version") {}
};

}  // namespace vcmp_lua

// The server's PluginFuncs, PluginCallbacks and PluginInfo grew over time:
// fields were appended (e.g. OnPlayerModuleList). An older server passes a
// smaller struct and reports its size in structSize, so a field may be read or
// written only when it lies inside that size.
#define VCMP_LUA_HAS_FIELD(ptr, Type, field) \
    ::vcmp_lua::FieldFits((ptr)->structSize, offsetof(Type, field), sizeof(Type::field))

namespace vcmp_lua {

// The server functions, read only through the structSize check.
class ServerApi {
public:
    ServerApi() = default;
    explicit ServerApi(PluginFuncs* funcs) noexcept : funcs_(funcs) {}

    [[nodiscard]] PluginFuncs* funcs() const noexcept { return funcs_; }

    // fn when the server provides it, else throws NotSupported.
    template <typename Fn>
    static Fn Require(Fn fn, const char* name) {
        if (fn == nullptr) {
            throw NotSupported(name);
        }
        return fn;
    }

private:
    PluginFuncs* funcs_ = nullptr;
};

}  // namespace vcmp_lua

// The server function `field` of a ServerApi, or nullptr when this server
// version does not have it.
#define VCMP_LUA_FIND(api, field)                                                  \
    (((api).funcs() != nullptr && VCMP_LUA_HAS_FIELD((api).funcs(), PluginFuncs, field)) \
         ? (api).funcs()->field                                                    \
         : nullptr)

// The server function `field` of a ServerApi; throws NotSupported when this
// server version does not have it.
#define VCMP_LUA_API(api, field) ::vcmp_lua::ServerApi::Require(VCMP_LUA_FIND(api, field), #field)
