// VC:MP plugin entry point. VcmpPluginInit is the only exported symbol
// (cmake/plugin.map) and every server callback is a noexcept wrapper: no
// exception may cross into the server's C code (plan B3.4).
#include <vcmp.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <utility>

#include "plugin/api_guard.hpp"
#include "runtime/config.hpp"
#include "runtime/libraries.hpp"
#include "runtime/log.hpp"
#include "runtime/runtime.hpp"
#include "runtime/version.hpp"

#if defined(_WIN32)
#define VCMP_LUA_EXPORT extern "C" __declspec(dllexport)
#else
#define VCMP_LUA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

namespace log = vcmp_lua::log;
using vcmp_lua::Runtime;

constexpr const char* kPluginName = "VCMP-Lua";
constexpr const char* kConfigFile = "luaconfig.lua";

// Owned here and deleted only by OnServerShutdown. If the server never calls
// it, the runtime is leaked on purpose: tearing Lua down from a static
// destructor or DllMain would run finalizers after the server is gone.
Runtime* g_runtime = nullptr;

// OnServerShutdown arrived while Lua was running; finish once it returns.
bool g_shutdown_deferred = false;

// Logs the exception being handled. Call only from a catch block.
void ReportException(const char* where) noexcept {
    try {
        throw;
    } catch (const std::exception& error) {
        log::Error("{}: {}", where, error.what());
    } catch (...) {
        log::Error("{}: unknown exception", where);
    }
}

void FinishShutdown() noexcept {
    Runtime* runtime = std::exchange(g_runtime, nullptr);
    g_shutdown_deferred = false;
    if (runtime == nullptr) {
        return;
    }
    runtime->Shutdown();
    delete runtime;
    vcmp_lua::libraries::Cleanup();
    log::Info("Shut down");
}

// Runs a deferred shutdown once no call into Lua is active (plan B3.1).
void AfterLuaCall() noexcept {
    if (g_shutdown_deferred && g_runtime != nullptr && !g_runtime->InLuaCall()) {
        FinishShutdown();
    }
}

// Every callback tolerates a missing or closed runtime: the server can still
// send events after OnServerShutdown.

uint8_t OnServerInitialise() noexcept {
    try {
        if (g_runtime != nullptr) {
            g_runtime->LoadScripts();
        }
    } catch (...) {
        ReportException("OnServerInitialise");
    }
    AfterLuaCall();
    return 1;
}

void OnServerFrame(float elapsed_seconds) noexcept {
    try {
        if (g_runtime != nullptr) {
            g_runtime->Frame(elapsed_seconds);
        }
    } catch (...) {
        ReportException("OnServerFrame");
    }
    AfterLuaCall();
}

void OnServerShutdown() noexcept {
    try {
        if (g_runtime == nullptr) {
            return;
        }
        if (g_runtime->InLuaCall()) {
            g_shutdown_deferred = true;
            return;
        }
        FinishShutdown();
    } catch (...) {
        ReportException("OnServerShutdown");
    }
}

void SetPluginName(PluginInfo* info) noexcept {
    const std::size_t length = std::min(std::strlen(kPluginName), sizeof(info->name) - 1);
    std::memcpy(info->name, kPluginName, length);
    info->name[length] = '\0';
}

}  // namespace

VCMP_LUA_EXPORT unsigned int VcmpPluginInit(PluginFuncs* funcs, PluginCallbacks* calls,
                                           PluginInfo* info) noexcept {
    try {
        log::Init();
        if (funcs == nullptr || calls == nullptr || info == nullptr) {
            log::Error("VcmpPluginInit: the server passed a null pointer");
            return 0;
        }
        if (g_runtime != nullptr) {
            log::Error("VcmpPluginInit: already initialised");
            return 0;
        }
        vcmp_lua::libraries::Init();

        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, name)) {
            SetPluginName(info);
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, pluginVersion)) {
            info->pluginVersion = vcmp_lua::kVersionNumber;
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, apiMajorVersion)) {
            info->apiMajorVersion = PLUGIN_API_MAJOR;
        }
        if (VCMP_LUA_HAS_FIELD(info, PluginInfo, apiMinorVersion)) {
            info->apiMinorVersion = PLUGIN_API_MINOR;
        }
        if (!VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerInitialise) ||
            !VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerShutdown) ||
            !VCMP_LUA_HAS_FIELD(calls, PluginCallbacks, OnServerFrame)) {
            log::Error("VcmpPluginInit: this server version has no OnServerFrame callback");
            vcmp_lua::libraries::Cleanup();
            return 0;
        }

        log::Info("VCMP-Lua {} with {}", vcmp_lua::kVersion, vcmp_lua::libraries::Versions());
        vcmp_lua::Config config = vcmp_lua::LoadConfig(kConfigFile);
        log::SetLevel(config.log_level);
        auto runtime = std::make_unique<Runtime>(std::move(config));

        // Last: the server must never call into a plugin that failed to start.
        calls->OnServerInitialise = &OnServerInitialise;
        calls->OnServerShutdown = &OnServerShutdown;
        calls->OnServerFrame = &OnServerFrame;
        g_runtime = runtime.release();
        return 1;
    } catch (...) {
        ReportException("VcmpPluginInit");
        vcmp_lua::libraries::Cleanup();
        return 0;
    }
}
