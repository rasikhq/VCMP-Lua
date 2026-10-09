// VC:MP plugin entry point. VcmpPluginInit is the only exported symbol
// (cmake/plugin.map). Everything else lives in vcmp_lua_core, which the tests
// link too (plugin/callbacks.cpp).
#include <vcmp.h>

#include "plugin/plugin.hpp"

#if defined(_WIN32)
#define VCMP_LUA_EXPORT extern "C" __declspec(dllexport)
#else
#define VCMP_LUA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

VCMP_LUA_EXPORT unsigned int VcmpPluginInit(PluginFuncs* funcs, PluginCallbacks* calls,
                                           PluginInfo* info) noexcept {
    try {
        return vcmp_lua::plugin::Init(funcs, calls, info, vcmp_lua::plugin::Options{});
    } catch (...) {
        return 0;  // Options{} could not allocate
    }
}
