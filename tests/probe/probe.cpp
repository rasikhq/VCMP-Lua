// Throwaway probe plugin. Runs on a real VC:MP 0.4 server and logs
// the order of server callbacks around entity creation and deletion, kicks,
// disconnects and shutdown, and whether OnEntityPoolChange fires for the
// plugin's own entities. The results are in docs/internals.md and settle the
// entity design. Not part of the plugin.
//
// Two copies are built: probe_a runs the scenario, probe_b only listens, so
// the log shows what one plugin sees of another plugin's entities.
//
// Scenario (probe_a): entity tests at start-up; then a person joins with a
// game client three times. Join 1 is kicked after 3 s; join 2 quits by
// itself; 5 s after join 3, the probe calls ShutdownServer() with that player
// still connected. PROBE_SHUTDOWN_AT=<seconds> (default 900) shuts the server
// down if nobody joins.
#include <vcmp.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#ifndef PROBE_NAME
#define PROBE_NAME "probe_a"
#endif
#ifndef PROBE_ACTIVE
#define PROBE_ACTIVE 1
#endif

#if defined(_WIN32)
#define PROBE_EXPORT extern "C" __declspec(dllexport)
#else
#define PROBE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

PluginFuncs* g_funcs = nullptr;
unsigned g_seq = 0;
unsigned long g_frame = 0;
double g_elapsed = 0.0;
const char* g_inside = nullptr;  // server function the probe is calling now
bool g_shutdown_seen = false;

void Log(const char* format, ...) {
    char text[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::printf("[%s #%u frame=%lu t=%.2f%s%s%s] %s\n", PROBE_NAME, ++g_seq, g_frame, g_elapsed,
                g_inside != nullptr ? " inside " : "", g_inside != nullptr ? g_inside : "",
                g_shutdown_seen ? " after-shutdown" : "", text);
    std::fflush(stdout);
}

// Calls a server function and marks callbacks that arrive during the call.
template <typename Fn, typename... Args>
auto Call(const char* name, Fn fn, Args... args) {
    g_inside = name;
    if constexpr (std::is_void_v<decltype(fn(args...))>) {
        fn(args...);
        g_inside = nullptr;
    } else {
        auto result = fn(args...);
        g_inside = nullptr;
        return result;
    }
}

const char* PoolName(vcmpEntityPool pool) {
    switch (pool) {
        case vcmpEntityPoolVehicle: return "vehicle";
        case vcmpEntityPoolObject: return "object";
        case vcmpEntityPoolPickup: return "pickup";
        case vcmpEntityPoolRadio: return "radio";
        case vcmpEntityPoolBlip: return "blip";
        case vcmpEntityPoolCheckPoint: return "checkpoint";
        default: return "?";
    }
}

// Scenario (probe_a only).

struct Kept {
    int32_t vehicle = -1;
    int32_t object = -1;
    int32_t pickup = -1;
    int32_t checkpoint = -1;
} g_kept;

int g_connections = 0;
int32_t g_player_to_kick = -1;
double g_kick_at = 0.0;
bool g_entities_tested = false;
bool g_shutdown_requested = false;
double g_shutdown_at = 900.0;
int g_frames_after_shutdown_request = 0;

void Exists(vcmpEntityPool pool, int32_t id, const char* when) {
    Log("CheckEntityExists(%s, %d) %s = %d (last error %d)", PoolName(pool), id, when,
        g_funcs->CheckEntityExists(pool, id), static_cast<int>(g_funcs->GetLastError()));
}

void TestEntities(const char* phase) {
    Log("--- entity test (%s) ---", phase);

    const int32_t vehicle = Call("CreateVehicle", g_funcs->CreateVehicle, 191, 1, -1000.0f, 200.0f, 11.0f, 0.0f, 1, 1);
    Log("CreateVehicle returned %d", vehicle);
    Exists(vcmpEntityPoolVehicle, vehicle, "after create");
    Log("DeleteVehicle returned %d", static_cast<int>(Call("DeleteVehicle", g_funcs->DeleteVehicle, vehicle)));
    Exists(vcmpEntityPoolVehicle, vehicle, "after delete");
    Log("DeleteVehicle again returned %d", static_cast<int>(Call("DeleteVehicle", g_funcs->DeleteVehicle, vehicle)));
    const int32_t reused = Call("CreateVehicle", g_funcs->CreateVehicle, 191, 1, -1000.0f, 205.0f, 11.0f, 0.0f, 1, 1);
    Log("CreateVehicle after delete returned %d (%s id)", reused, reused == vehicle ? "same" : "different");
    Log("DeleteVehicle returned %d", static_cast<int>(Call("DeleteVehicle", g_funcs->DeleteVehicle, reused)));

    const int32_t object = Call("CreateObject", g_funcs->CreateObject, 600, 1, -1000.0f, 210.0f, 11.0f, 255);
    Log("CreateObject returned %d", object);
    Log("DeleteObject returned %d", static_cast<int>(Call("DeleteObject", g_funcs->DeleteObject, object)));

    const int32_t pickup = Call("CreatePickup", g_funcs->CreatePickup, 336, 1, 1, -1000.0f, 215.0f, 11.0f, 255, static_cast<uint8_t>(0));
    Log("CreatePickup returned %d", pickup);
    Log("DeletePickup returned %d", static_cast<int>(Call("DeletePickup", g_funcs->DeletePickup, pickup)));

    const int32_t checkpoint = Call("CreateCheckPoint", g_funcs->CreateCheckPoint, -1, 1, static_cast<uint8_t>(0), -1000.0f, 220.0f, 11.0f, 255, 0, 0, 255, 2.0f);
    Log("CreateCheckPoint returned %d", checkpoint);
    Log("DeleteCheckPoint returned %d", static_cast<int>(Call("DeleteCheckPoint", g_funcs->DeleteCheckPoint, checkpoint)));

    const int32_t blip = Call("CreateCoordBlip", g_funcs->CreateCoordBlip, -1, 1, -1000.0f, 225.0f, 11.0f, 1, 0xFF0000FFu, 0);
    Log("CreateCoordBlip returned %d", blip);
    Log("DestroyCoordBlip returned %d", static_cast<int>(Call("DestroyCoordBlip", g_funcs->DestroyCoordBlip, blip)));

    Log("AddRadioStream(5) returned %d", static_cast<int>(Call("AddRadioStream", g_funcs->AddRadioStream, 5, "probe", "http://127.0.0.1/none", static_cast<uint8_t>(0))));
    Log("RemoveRadioStream(5) returned %d", static_cast<int>(Call("RemoveRadioStream", g_funcs->RemoveRadioStream, 5)));

    Log("RegisterKeyBind(0) returned %d", static_cast<int>(Call("RegisterKeyBind", g_funcs->RegisterKeyBind, 0, static_cast<uint8_t>(0), 0x42, 0, 0)));
    Log("RemoveKeyBind(0) returned %d", static_cast<int>(Call("RemoveKeyBind", g_funcs->RemoveKeyBind, 0)));
}

void KeepEntities() {
    g_kept.vehicle = Call("CreateVehicle", g_funcs->CreateVehicle, 191, 1, -1000.0f, 230.0f, 11.0f, 0.0f, 1, 1);
    g_kept.object = Call("CreateObject", g_funcs->CreateObject, 600, 1, -1000.0f, 235.0f, 11.0f, 255);
    g_kept.pickup = Call("CreatePickup", g_funcs->CreatePickup, 336, 1, 1, -1000.0f, 240.0f, 11.0f, 255, static_cast<uint8_t>(0));
    g_kept.checkpoint = Call("CreateCheckPoint", g_funcs->CreateCheckPoint, -1, 1, static_cast<uint8_t>(0), -1000.0f, 245.0f, 11.0f, 255, 0, 0, 255, 2.0f);
    Log("kept for shutdown: vehicle %d, object %d, pickup %d, checkpoint %d", g_kept.vehicle,
        g_kept.object, g_kept.pickup, g_kept.checkpoint);
}

void PoolBounds() {
    const vcmpEntityPool pools[] = {vcmpEntityPoolVehicle, vcmpEntityPoolObject, vcmpEntityPoolPickup,
                                    vcmpEntityPoolRadio, vcmpEntityPoolBlip, vcmpEntityPoolCheckPoint};
    for (vcmpEntityPool pool : pools) {
        int32_t index = 0;
        for (; index < 100000; ++index) {
            g_funcs->CheckEntityExists(pool, index);
            if (g_funcs->GetLastError() == vcmpErrorArgumentOutOfBounds) {
                break;
            }
        }
        Log("pool %s: CheckEntityExists is out of bounds from index %d", PoolName(pool), index);
    }
}

// Callbacks.

uint8_t OnServerInitialise() {
    Log("OnServerInitialise");
    if (PROBE_ACTIVE) {
        TestEntities("inside OnServerInitialise");
    }
    return 1;
}

void OnServerShutdown() {
    Log("OnServerShutdown");
    g_shutdown_seen = true;
}

void OnServerFrame(float elapsed) {
    ++g_frame;
    g_elapsed += elapsed;
    if (g_frame == 1) {
        Log("OnServerFrame (first, elapsed %.4f s)", elapsed);
    }
    if (g_shutdown_seen && g_frame % 100 == 0) {
        Log("OnServerFrame still called after OnServerShutdown");
    }
    if (!PROBE_ACTIVE) {
        return;
    }
    if (g_shutdown_requested && g_frames_after_shutdown_request < 3) {
        ++g_frames_after_shutdown_request;
        Log("OnServerFrame after ShutdownServer() (%d)", g_frames_after_shutdown_request);
    }
    if (!g_entities_tested && g_elapsed >= 0.5) {
        g_entities_tested = true;
        TestEntities("inside OnServerFrame");
        KeepEntities();
        PoolBounds();
    }
    if (g_player_to_kick >= 0 && g_elapsed >= g_kick_at) {
        const int32_t player = g_player_to_kick;
        g_player_to_kick = -1;
        Log("KickPlayer(%d) returned %d", player, static_cast<int>(Call("KickPlayer", g_funcs->KickPlayer, player)));
        Log("IsPlayerConnected(%d) right after KickPlayer = %d", player, g_funcs->IsPlayerConnected(player));
    }
    if (!g_shutdown_requested && g_elapsed >= g_shutdown_at) {
        g_shutdown_requested = true;
        Log("frames so far: %lu", g_frame);
        Call("ShutdownServer", g_funcs->ShutdownServer);
        Log("ShutdownServer returned");
    }
}

uint8_t OnIncomingConnection(char* name, size_t, const char*, const char* ip) {
    Log("OnIncomingConnection name=%s ip=%s", name, ip);
    return 1;
}

void OnPlayerConnect(int32_t player) {
    char name[64] = "";
    g_funcs->GetPlayerName(player, name, sizeof(name));
    Log("OnPlayerConnect %d (%s), IsPlayerConnected = %d", player, name, g_funcs->IsPlayerConnected(player));
    ++g_connections;
    Log("connection %d", g_connections);
    if (PROBE_ACTIVE && g_connections == 1) {
        g_player_to_kick = player;
        g_kick_at = g_elapsed + 3.0;
    } else if (PROBE_ACTIVE && g_connections == 3) {
        g_shutdown_at = g_elapsed + 5.0;
    }
}

void OnPlayerDisconnect(int32_t player, vcmpDisconnectReason reason) {
    char name[64] = "";
    const vcmpError name_error = g_funcs->GetPlayerName(player, name, sizeof(name));
    Log("OnPlayerDisconnect %d (%s, GetPlayerName error %d) reason %d, IsPlayerConnected = %d", player,
        name, static_cast<int>(name_error), static_cast<int>(reason), g_funcs->IsPlayerConnected(player));
}

uint8_t OnPlayerRequestClass(int32_t player, int32_t offset) {
    Log("OnPlayerRequestClass %d offset %d", player, offset);
    return 1;
}

uint8_t OnPlayerRequestSpawn(int32_t player) {
    Log("OnPlayerRequestSpawn %d", player);
    return 1;
}

void OnPlayerSpawn(int32_t player) { Log("OnPlayerSpawn %d", player); }

void OnPlayerDeath(int32_t player, int32_t killer, int32_t reason, vcmpBodyPart part) {
    Log("OnPlayerDeath %d killer %d reason %d part %d", player, killer, reason, static_cast<int>(part));
}

void OnPlayerEnterVehicle(int32_t player, int32_t vehicle, int32_t slot) {
    Log("OnPlayerEnterVehicle %d vehicle %d slot %d", player, vehicle, slot);
}

void OnPlayerExitVehicle(int32_t player, int32_t vehicle) {
    Log("OnPlayerExitVehicle %d vehicle %d", player, vehicle);
}

void OnVehicleExplode(int32_t vehicle) { Log("OnVehicleExplode %d", vehicle); }
void OnVehicleRespawn(int32_t vehicle) { Log("OnVehicleRespawn %d", vehicle); }
void OnPickupRespawn(int32_t pickup) { Log("OnPickupRespawn %d", pickup); }

void OnEntityPoolChange(vcmpEntityPool pool, int32_t id, uint8_t deleted) {
    const uint8_t exists = g_funcs->CheckEntityExists(pool, id);
    Log("OnEntityPoolChange %s %d %s, CheckEntityExists = %d", PoolName(pool), id,
        deleted != 0 ? "deleted" : "created", exists);
}

uint8_t OnPluginCommand(uint32_t command, const char* message) {
    Log("OnPluginCommand %u %s", command, message != nullptr ? message : "(null)");
    return 1;
}

}  // namespace

PROBE_EXPORT unsigned int VcmpPluginInit(PluginFuncs* funcs, PluginCallbacks* calls, PluginInfo* info) {
    g_funcs = funcs;
    std::snprintf(info->name, sizeof(info->name), "%s", PROBE_NAME);
    info->pluginVersion = 1;
    info->apiMajorVersion = PLUGIN_API_MAJOR;
    info->apiMinorVersion = 0;  // the server refuses 2.1 (docs/internals.md)
    if (const char* at = std::getenv("PROBE_SHUTDOWN_AT")) {
        g_shutdown_at = std::atof(at);
    }
    Log("VcmpPluginInit: server version %u, PluginFuncs %u bytes (header %zu), PluginCallbacks %u bytes "
        "(header %zu), PluginInfo %u bytes (header %zu)",
        funcs->GetServerVersion(), funcs->structSize, sizeof(PluginFuncs), calls->structSize,
        sizeof(PluginCallbacks), info->structSize, sizeof(PluginInfo));

    calls->OnServerInitialise = OnServerInitialise;
    calls->OnServerShutdown = OnServerShutdown;
    calls->OnServerFrame = OnServerFrame;
    calls->OnIncomingConnection = OnIncomingConnection;
    calls->OnPlayerConnect = OnPlayerConnect;
    calls->OnPlayerDisconnect = OnPlayerDisconnect;
    calls->OnPlayerRequestClass = OnPlayerRequestClass;
    calls->OnPlayerRequestSpawn = OnPlayerRequestSpawn;
    calls->OnPlayerSpawn = OnPlayerSpawn;
    calls->OnPlayerDeath = OnPlayerDeath;
    calls->OnPlayerEnterVehicle = OnPlayerEnterVehicle;
    calls->OnPlayerExitVehicle = OnPlayerExitVehicle;
    calls->OnVehicleExplode = OnVehicleExplode;
    calls->OnVehicleRespawn = OnVehicleRespawn;
    calls->OnPickupRespawn = OnPickupRespawn;
    calls->OnEntityPoolChange = OnEntityPoolChange;
    calls->OnPluginCommand = OnPluginCommand;
    return 1;
}
