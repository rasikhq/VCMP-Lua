#pragma once

#include <cstddef>
#include <span>

namespace vcmp_lua::embedded {

// A pure-Lua module compiled into the plugin (cmake/EmbedLua.cmake).
struct Module {
    const char* name;       // require() name, e.g. "socket.http"
    const char* chunkname;  // "@builtin/<name>.lua"
    const unsigned char* source;
    std::size_t size;
};

std::span<const Module> Modules() noexcept;

}  // namespace vcmp_lua::embedded
