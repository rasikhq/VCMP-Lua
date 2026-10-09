// plugin_host: loads the built plugin the way the VC:MP server does, hands it
// a fake server API and drives it through init -> frames -> shutdown ->
// unload. It links nothing from the plugin; it sees only the exported symbol.
//
// usage: plugin_host <plugin> [options]
//   --frames N             OnServerFrame calls between init and shutdown (60)
//   --server new|old|ancient
//                          new:     current struct sizes
//                          old:     PluginCallbacks without OnPlayerModuleList
//                          ancient: PluginCallbacks that ends before
//                                   OnServerFrame, PluginInfo without a name
//   --expect-init-failure  VcmpPluginInit must return 0
//   --expect FILE          after unloading, FILE must start with "PASS". The
//                          test script writes it; it is deleted before loading.
//
// Prints "HOST PASS" and exits with 0 when every check passed.
#include <vcmp.h>

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

int g_failures = 0;

void Fail(const std::string& message) {
    std::fprintf(stderr, "HOST FAIL: %s\n", message.c_str());
    std::fflush(stderr);
    ++g_failures;
}

void Step(const std::string& message) {
    std::printf("host: %s\n", message.c_str());
    std::fflush(stdout);
}

// Loading the plugin.

class Library {
public:
    bool Open(const std::string& path) {
#if defined(_WIN32)
        handle_ = LoadLibraryA(path.c_str());
        if (handle_ == nullptr) {
            Fail("LoadLibrary failed with error " + std::to_string(GetLastError()));
        }
#else
        handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle_ == nullptr) {
            Fail(std::string("dlopen failed: ") + dlerror());
        }
#endif
        path_ = path;
        return handle_ != nullptr;
    }

    void* Symbol(const char* name) const {
#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
        return dlsym(handle_, name);
#endif
    }

    // Unloads the plugin and checks that it is really gone: nothing (such as
    // OpenSSL pinning itself) may keep it mapped.
    void Close() {
#if defined(_WIN32)
        if (!FreeLibrary(static_cast<HMODULE>(handle_))) {
            Fail("FreeLibrary failed with error " + std::to_string(GetLastError()));
        }
        if (GetModuleHandleA(path_.c_str()) != nullptr) {
            Fail("the plugin is still loaded after FreeLibrary");
        }
#else
        if (dlclose(handle_) != 0) {
            Fail(std::string("dlclose failed: ") + dlerror());
        }
        if (void* again = dlopen(path_.c_str(), RTLD_NOW | RTLD_NOLOAD)) {
            Fail("the plugin is still loaded after dlclose");
            dlclose(again);
        }
#endif
        handle_ = nullptr;
    }

private:
    void* handle_ = nullptr;
    std::string path_;
};

// Fake server API.

const auto g_start = std::chrono::steady_clock::now();

uint32_t FakeGetServerVersion() { return 0x00040701; }

vcmpError FakeGetServerSettings(ServerSettings* settings) {
    if (settings == nullptr) {
        return vcmpErrorNullArgument;
    }
    std::memset(settings, 0, sizeof(*settings));
    settings->structSize = sizeof(*settings);
    std::snprintf(settings->serverName, sizeof(settings->serverName), "plugin_host");
    settings->maxPlayers = 100;
    settings->port = 8192;
    return vcmpErrorNone;
}

uint32_t FakeGetNumberOfPlugins() { return 1; }

uint64_t FakeGetTime() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::steady_clock::now() - g_start)
                                     .count());
}

vcmpError FakeLogMessage(const char* format, ...) {
    if (format == nullptr) {
        return vcmpErrorNullArgument;
    }
    std::va_list args;
    va_start(args, format);
    std::printf("server log: ");
    std::vprintf(format, args);
    std::printf("\n");
    va_end(args);
    return vcmpErrorNone;
}

vcmpError FakeGetLastError() { return vcmpErrorNone; }

// An empty server: no players, no entities, no key binds.
uint8_t FakeIsPlayerConnected(int32_t) { return 0; }

uint8_t FakeCheckEntityExists(vcmpEntityPool, int32_t) { return 0; }

vcmpError FakeGetKeyBindData(int32_t, uint8_t*, int32_t*, int32_t*, int32_t*) {
    return vcmpErrorNoSuchEntity;
}

// Every other PluginFuncs slot points at a trap that records the call.
std::vector<std::size_t> g_unexpected_calls;

template <std::size_t Slot>
uint64_t Trap() {
    g_unexpected_calls.push_back(Slot);
    return 0;
}

constexpr std::size_t kFirstSlot = offsetof(PluginFuncs, GetServerVersion);
constexpr std::size_t kSlotCount = (sizeof(PluginFuncs) - kFirstSlot) / sizeof(void*);
static_assert((sizeof(PluginFuncs) - kFirstSlot) % sizeof(void*) == 0,
              "PluginFuncs holds only function pointers after structSize");

using TrapFn = uint64_t (*)();

template <std::size_t... Slot>
constexpr std::array<TrapFn, sizeof...(Slot)> MakeTraps(std::index_sequence<Slot...>) {
    return {&Trap<Slot>...};
}

constexpr auto kTraps = MakeTraps(std::make_index_sequence<kSlotCount>{});

void FillFuncs(PluginFuncs& funcs, uint32_t struct_size) {
    std::memset(&funcs, 0, sizeof(funcs));
    for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
        const TrapFn trap = kTraps[slot];
        std::memcpy(reinterpret_cast<unsigned char*>(&funcs) + kFirstSlot + slot * sizeof(void*),
                    &trap, sizeof(trap));
    }
    funcs.structSize = struct_size;
    funcs.GetServerVersion = &FakeGetServerVersion;
    funcs.GetServerSettings = &FakeGetServerSettings;
    funcs.GetNumberOfPlugins = &FakeGetNumberOfPlugins;
    funcs.GetTime = &FakeGetTime;
    funcs.LogMessage = &FakeLogMessage;
    funcs.GetLastError = &FakeGetLastError;
    funcs.IsPlayerConnected = &FakeIsPlayerConnected;
    funcs.CheckEntityExists = &FakeCheckEntityExists;
    funcs.GetKeyBindData = &FakeGetKeyBindData;
}

// A struct inside a larger buffer. Bytes past the reported structSize hold a
// canary; the plugin must never write there.
template <typename T>
class GuardedStruct {
public:
    explicit GuardedStruct(uint32_t struct_size) : size_(struct_size) {
        std::memset(bytes(), kCanary, kBytes);
        std::memset(bytes(), 0, struct_size);
        get()->structSize = struct_size;
    }

    T* get() { return reinterpret_cast<T*>(storage_.data()); }

    void CheckCanary(const char* name) {
        for (std::size_t i = size_; i < kBytes; ++i) {
            if (bytes()[i] != kCanary) {
                Fail(std::string("the plugin wrote past structSize of ") + name + " at byte " +
                     std::to_string(i));
                return;
            }
        }
    }

private:
    static constexpr unsigned char kCanary = 0xAB;
    static constexpr std::size_t kBytes = sizeof(T) + 64;

    unsigned char* bytes() { return reinterpret_cast<unsigned char*>(storage_.data()); }

    // uint64_t elements give the buffer the alignment of the server structs.
    std::array<uint64_t, (kBytes + sizeof(uint64_t) - 1) / sizeof(uint64_t)> storage_{};
    uint32_t size_;
};

// Expected result files.

void CheckResultFile(const std::string& path) {
    std::ifstream file(path);
    std::string first_line;
    if (!file || !std::getline(file, first_line)) {
        Fail(path + " was not written");
        return;
    }
    if (first_line.rfind("PASS", 0) != 0) {
        std::string rest((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        Fail(path + ": " + first_line + "\n" + rest);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: plugin_host <plugin> [options]\n");
        return 2;
    }
    const std::string plugin_path = argv[1];
    int frames = 60;
    std::string server = "new";
    bool expect_init_failure = false;
    std::vector<std::string> expected_files;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--frames" && i + 1 < argc) {
            frames = std::stoi(argv[++i]);
        } else if (arg == "--server" && i + 1 < argc) {
            server = argv[++i];
        } else if (arg == "--expect-init-failure") {
            expect_init_failure = true;
        } else if (arg == "--expect" && i + 1 < argc) {
            expected_files.emplace_back(argv[++i]);
        } else {
            std::fprintf(stderr, "plugin_host: unknown argument %s\n", arg.c_str());
            return 2;
        }
    }

    uint32_t funcs_size = static_cast<uint32_t>(sizeof(PluginFuncs));
    uint32_t calls_size = static_cast<uint32_t>(sizeof(PluginCallbacks));
    uint32_t info_size = static_cast<uint32_t>(sizeof(PluginInfo));
    if (server == "old") {
        funcs_size = static_cast<uint32_t>(offsetof(PluginFuncs, SetPlayerDrunkHandling));
        calls_size = static_cast<uint32_t>(offsetof(PluginCallbacks, OnPlayerModuleList));
    } else if (server == "ancient") {
        funcs_size = static_cast<uint32_t>(offsetof(PluginFuncs, SetPlayerDrunkHandling));
        calls_size = static_cast<uint32_t>(offsetof(PluginCallbacks, OnServerFrame));
        info_size = static_cast<uint32_t>(offsetof(PluginInfo, name));
    } else if (server != "new") {
        std::fprintf(stderr, "plugin_host: unknown server kind %s\n", server.c_str());
        return 2;
    }

    for (const std::string& file : expected_files) {
        std::remove(file.c_str());
    }

    Library plugin;
    Step("loading " + plugin_path);
    if (!plugin.Open(plugin_path)) {
        return 1;
    }
    using InitFn = unsigned int (*)(PluginFuncs*, PluginCallbacks*, PluginInfo*);
    const auto init = reinterpret_cast<InitFn>(plugin.Symbol("VcmpPluginInit"));
    if (init == nullptr) {
        Fail("VcmpPluginInit is not exported");
        return 1;
    }

    PluginFuncs funcs;
    FillFuncs(funcs, funcs_size);
    GuardedStruct<PluginCallbacks> calls(calls_size);
    GuardedStruct<PluginInfo> info(info_size);

    Step("VcmpPluginInit (" + server + " server)");
    const unsigned int result = init(&funcs, calls.get(), info.get());
    calls.CheckCanary("PluginCallbacks");
    info.CheckCanary("PluginInfo");

    if (expect_init_failure) {
        if (result != 0) {
            Fail("VcmpPluginInit returned " + std::to_string(result) + ", expected 0");
        }
    } else if (result != 1) {
        Fail("VcmpPluginInit returned " + std::to_string(result) + ", expected 1");
    } else {
        if (info_size >= offsetof(PluginInfo, name) + sizeof(PluginInfo::name)) {
            const char* name = info.get()->name;
            const void* nul = std::memchr(name, '\0', sizeof(PluginInfo::name));
            if (nul == nullptr || name[0] == '\0') {
                Fail("PluginInfo.name is empty or not NUL-terminated");
            } else {
                Step(std::string("plugin name: ") + name);
            }
        }
        // The 0.4 server refuses a plugin whose API version is above its own,
        // which is 2.0 (docs/internals.md).
        if (info_size >= offsetof(PluginInfo, apiMinorVersion) + sizeof(uint16_t) &&
            (info.get()->apiMajorVersion != 2 || info.get()->apiMinorVersion != 0)) {
            Fail("PluginInfo reports API " + std::to_string(info.get()->apiMajorVersion) + "." +
                 std::to_string(info.get()->apiMinorVersion) + ", expected 2.0");
        }
        PluginCallbacks* callbacks = calls.get();
        if (callbacks->OnServerInitialise == nullptr || callbacks->OnServerFrame == nullptr ||
            callbacks->OnServerShutdown == nullptr) {
            Fail("the plugin did not set OnServerInitialise, OnServerFrame and OnServerShutdown");
        } else {
            Step("OnServerInitialise");
            if (callbacks->OnServerInitialise() != 1) {
                Fail("OnServerInitialise did not return 1");
            }
            Step("OnServerFrame x" + std::to_string(frames));
            for (int frame = 0; frame < frames; ++frame) {
                callbacks->OnServerFrame(1.0f / 60.0f);
            }
            Step("OnServerShutdown");
            callbacks->OnServerShutdown();
            // The server can still send events after shutdown.
            Step("events after OnServerShutdown");
            callbacks->OnServerFrame(1.0f / 60.0f);
            callbacks->OnServerShutdown();
        }
    }

    if (!g_unexpected_calls.empty()) {
        Fail("the plugin called " + std::to_string(g_unexpected_calls.size()) +
             " fake server functions that are not implemented (first slot: " +
             std::to_string(g_unexpected_calls.front()) + ")");
    }

    Step("unloading");
    plugin.Close();

    for (const std::string& file : expected_files) {
        CheckResultFile(file);
    }

    if (g_failures != 0) {
        std::printf("HOST FAILED (%d)\n", g_failures);
        return 1;
    }
    std::printf("HOST PASS\n");
    return 0;
}
