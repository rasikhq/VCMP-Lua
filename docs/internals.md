# Internals

Facts about the VC:MP server that the v2 runtime is built on, measured in
phase 1. They settle the entity design (plan B4) and the shutdown order
(plan B3.1). The last section describes how the runtime (phase 2) uses them.

## How the facts were measured

- **Server:** official VC:MP 0.4 Linux x64 server (`mpsvrrel64`, archive
  `VCMP04_server_v46_linux64.zip`, `GetServerVersion()` = 67710). It runs in
  `ubuntu:24.04` (it needs glibc 2.38 and libstdc++) as uid 1000.
- **Probe:** `tests/probe` builds two throwaway plugins. `probe_a` runs the
  scenario and `probe_b` only listens, so the log also shows what one plugin
  sees of another plugin's entities. Every line says whether the callback
  arrived *inside* a server function the probe was calling.
- **Real plugin:** `LuaPlugin_x64.so` was loaded next to `probe_a` with
  `tests/host/smoke.lua` as its script.
- **To run it again:** build `tests/probe` (see its CMakeLists.txt), put
  `mpsvrrel64`, `server.cfg` (`port 5192`, `plugins probe_a probe_b`) and
  `plugins/` in one directory, and start the server with
  `-p 5192:5192/udp`. Join three times: the probe kicks join 1 after 3 s,
  quit join 2 yourself, and 5 s after join 3 it calls `ShutdownServer()`.

## Plugin API structs

| Struct | Size reported by the server | Size in `third_party/vcmp/vcmp.h` |
|---|---|---|
| `PluginFuncs` | 2360 bytes | 2360 bytes |
| `PluginCallbacks` | 376 bytes | 376 bytes |
| `PluginInfo` | 48 bytes | 48 bytes |

The server had one more function and one more callback than the phase 1
SDK header (API 2.0). Phase 3 replaced it with the API 2.1 header that SqMod
ships (`module/VCMP/vcmp21.h`), whose structs are exactly the server's size:
it appends `GetNetworkStatistics` and `OnEntityStreamingChange`, and adds
`vcmpEntityPoolPlayer`. Fields are only ever appended, and the `structSize`
checks (plan B3.5) handle older and newer servers in both directions.

## Entities

Vehicles, objects, pickups, checkpoints and coordinate blips behave the same:

- `OnEntityPoolChange` fires **synchronously inside** `Create*` and `Delete*`
  (`DestroyCoordBlip` for blips), before the call returns.
- It fires for the plugin's **own** entities, first for the plugin that made
  the call and then for every other plugin, in load order. Every plugin sees
  every entity.
- In the "created" event, `CheckEntityExists` already returns 1.
- In the "deleted" event, `CheckEntityExists` still returns 1 for vehicles,
  objects, pickups and checkpoints, but 0 for blips. After `Delete*`
  returns it is 0. The event, not `CheckEntityExists`, is the source of truth.
- **IDs are reused at once:** create, delete, create returns the same id. A
  handle therefore needs a generation, not just an id (`EntityRef{id, gen}`).
- The first id is 1 for vehicles and 0 for objects, pickups, checkpoints and
  blips.
- Deleting an entity twice returns `vcmpErrorNoSuchEntity`.
- `CheckEntityExists` never reports `vcmpErrorArgumentOutOfBounds`, for any
  pool, up to index 100000 (`GetLastError()` stays `vcmpErrorNone`). Start-up
  enumeration cannot stop at "out of bounds"; it needs the pool limits.
- Creating entities inside `OnServerInitialise` works, and the pool events
  arrive there too.

Not entities in this sense:

- Radio streams raise no pool events. `AddRadioStream(5, ...)` returned
  `vcmpErrorArgumentOutOfBounds`.
- Key binds raise no pool events (`RegisterKeyBind`, `RemoveKeyBind`).

Consequences for B4: the pool feed is complete, including other plugins'
entities, so "adopt on first sight" works from `OnEntityPoolChange` alone.
Adopt and release must be idempotent, because the event arrives while the
binding's `Create*`/`Delete*` call is still on the stack.

## Frames

An idle server calls `OnServerFrame` about 168 times per second (1344 frames
in 8 s). The first frame reports 0 s elapsed.

## Shutdown

- `ShutdownServer()` returns at once. `OnServerShutdown` arrives later in the
  same frame, after the current `OnServerFrame` callback has returned, so a
  script that shuts the server down does not re-enter Lua. The runtime still
  defers a shutdown requested during a Lua call (B3.1), as a safeguard.
- Plugins receive `OnServerShutdown` in load order.
- **After** `OnServerShutdown`, the server deletes the remaining entities and
  sends an `OnEntityPoolChange` "deleted" event for each (pickups, objects,
  checkpoints, then vehicles). Callbacks must tolerate a closed runtime.
- No `OnServerFrame` follows `OnServerShutdown`. The process exits with 0.
- **Ctrl+C (SIGINT)** gives the same clean sequence.
- **SIGTERM** ends the process at once (exit status 143) without
  `OnServerShutdown`: scripts get no shutdown event. Inside a container the
  server runs as PID 1, where SIGTERM is ignored, so `docker stop` kills it
  after its timeout (exit status 137). Stop servers with Ctrl+C, or from a
  script.
- The server refuses to run as root unless started with
  `-allow-server-runas-root`.

## LuaPlugin_x64 in the real server

The phase 1 plugin loads (`Loaded plugin: LuaPlugin_x64`), runs every check
of `tests/host/smoke.lua` (`SMOKE PASS`), shuts down on `ShutdownServer()`
and on Ctrl+C with its Lua finalizers running, and the server exits with 0.

## Players

Measured with a real VC:MP client joining from Windows three times: the
probe kicked join 1 after 3 s, join 2 quit by itself, and 5 s after join 3
the probe called `ShutdownServer()` with the player still online.

- **Join:** `OnIncomingConnection` reaches every plugin, then
  `OnPlayerConnect` reaches every plugin, in the same frame. During
  `OnPlayerConnect`, `IsPlayerConnected` returns 1. `OnPlayerRequestClass`
  follows about 0.1 s later. Player ids are reused: the client got id 0 on
  every join.
- **Kick:** `OnPlayerDisconnect` with reason 2 (`vcmpDisconnectReasonKick`)
  fires **synchronously inside** `KickPlayer`, for every plugin. During the
  event `IsPlayerConnected` returns 1 and `GetPlayerName` works. Right after
  `KickPlayer` returns, `IsPlayerConnected` returns 0.
- **Quit:** `OnPlayerDisconnect` with reason 1 (`vcmpDisconnectReasonQuit`),
  in the frame; the player still counts as connected during the event.
- **Shutdown with a player online:** `ShutdownServer()` returns, then
  `OnServerShutdown` reaches every plugin, then the pool "deleted" events
  for the remaining entities, and only then `OnPlayerDisconnect` with
  reason 0 (`vcmpDisconnectReasonTimeout`).

Consequences:

- B4's rule "players are invalidated after the disconnect dispatch" matches
  the server: the player is valid during the event and gone right after it.
- A script's kick call runs the disconnect handlers re-entrantly, inside the
  binding. The event bus must allow nested dispatch (B3.6), and the kick
  binding must not use the player after the server call returns.
- Players are still connected during `OnServerShutdown`, and their
  disconnects arrive after the runtime has closed. The runtime therefore
  dispatches them itself (see "Runtime" below).
- In a container on macOS the server sees the NAT gateway's address
  (192.168.215.1), not the client's, so player IPs are not meaningful there.

## MySQL gate

Phase 1 kept MySQL only if all four gate conditions held. They do
(`tests/integration/compose.yml`, service `mysql-gate`):

| Check | Result |
|---|---|
| `caching_sha2_password` (static) against MySQL 8.4.11: full authentication over TLS, cached fast authentication, and full authentication over plain TCP with the RSA key exchange | pass |
| `mysql_native_password` against MariaDB 11.8.9, default TLS settings | pass |
| No authentication plugin is loaded from disk: fake plugins planted in `$MARIADB_PLUGIN_DIR` and in the connector's default directory are never loaded; an ed25519 account fails with "Plugin client_ed25519 could not be loaded" | pass |
| Timeouts patch: a server that never answers gives up after 2.0 s (`connect_timeout = 2`), `SELECT SLEEP(20)` gives up after 2.0 s (`read_timeout = 2`), `write_timeout` is accepted, invalid options raise errors | pass |

Behaviour to document for users:

- MariaDB Connector/C 3.4 uses TLS and verifies the server certificate by
  default. A self-signed certificate is accepted only for local connections
  and for password-hash authentication methods (`mysql_native_password`,
  ed25519). MySQL 8.4's generated certificate is therefore refused for
  `caching_sha2_password` over the network; use `ssl = "require"` (encrypted,
  not verified) or a certificate the client can verify (`ssl = "verify"`,
  `ssl_ca`).
- ed25519 accounts are not supported: the connector cannot link its client
  plugin statically, and the plugin never loads one from disk.
- `LIBMYSQL_PLUGINS`, an environment variable the operator controls, can
  still make the connector load plugins when it initialises.

## Runtime

How the phase 2 core (`src/core`, `src/runtime`, `src/plugin`) behaves. The
unit tests in `tests/unit` check each point against `FakeServer`, an
in-memory server that reproduces the behaviour measured above.

### Lifetime

- `VcmpPluginInit` reads `luaconfig.lua` and creates the `Runtime`; scripts
  run in `OnServerInitialise`, after the runtime adopts the players and
  entities that already exist. `onServerInit` follows the scripts.
- Each frame runs the frame pump (timers; HTTP and Copas from phase 4), then
  `onServerFrame(elapsedSeconds)`.
- `OnServerShutdown`: `onServerShutdown`, then `onPlayerDisconnect` for
  every player still online, with reason 0 (`vcmpDisconnectReasonTimeout`,
  the reason the server itself reports later), then the shutdown. Scripts
  can save player data in their disconnect handler in every case. The
  server's own disconnects arrive after the runtime has closed and are
  ignored.
- Shutdown order: mark closing (bindings raise "runtime shutting down", no
  new entity handles); release every Lua reference C++ holds (handlers,
  timers, entity handles and data tables); close the Lua state, so `__gc`
  runs while the subsystems still exist; destroy the subsystems.
- If `OnServerShutdown` arrives during a call into Lua, the shutdown runs
  when that call returns. (No frame follows `OnServerShutdown`, so it cannot
  wait for the end of the frame.)
- After a Lua panic the runtime is dead: every callback does nothing, and
  the Lua state is leaked at shutdown instead of closed.

### Reload

`Server.reload()` sets a flag. At the end of the frame, once no call into Lua
is active, the plugin reads `luaconfig.lua` again (if it is now invalid, the
error is logged and the old scripts keep running), deletes the vehicles,
objects, pickups, checkpoints and blips the old runtime created, closes the
old Lua state and starts a new runtime, which adopts the existing players and
entities and runs the scripts. Player classes cannot be removed through the
server API, so they stay. Phase 3 adds key binds to what a reload deletes.

### Events

- Built-in events are indexed by an enum; `Event.create` adds custom events
  after them. Handlers run in bind order.
- A dispatch calls only the handlers that existed when it started. A
  handler bound during the dispatch runs from the next one on; a handler
  unbound before its turn does not run.
- `Event.cancel()` stops the remaining handlers of the innermost active
  dispatch (from phase 3 it also makes a cancellable server callback return
  0). Outside a dispatch it raises an error. `Event.trigger` returns false
  when a handler cancelled.
- A handler that raises an error is logged with its traceback, and the next
  handler runs.
- Server callbacks can nest: `KickPlayer` reports the disconnect
  synchronously, so the disconnect handlers run inside the kicking handler.

### Timers

- `Timer.create(fn, intervalMs, repeats, ...)`: `repeats` is -1 (forever) or
  a positive count; the interval is 0 to 2^40 ms. Integral floats such as
  `1000 / 2` are accepted.
- Times are 64-bit `steady_clock` milliseconds, so they never wrap.
- A tick collects the due timers first, then calls them. A timer created in
  a callback waits for the next tick; one destroyed in a callback does not
  run.
- Fixed rate, without catch-up: after a long stall a timer runs once, then
  keeps its rate from that moment.
- `thisTimer` is the running timer, as in v1.

### Entities

- One pool per kind with the server's limits (players 100, vehicles
  1-1000, objects 3000, pickups 2000, checkpoints 2000, blips 100). Ids
  outside a pool are logged and ignored.
- A slot holds a generation, the cached Lua handle (created when Lua first
  sees the entity), the `data` table and whether this runtime created the
  entity. The generation changes once per lifetime.
- Lua gets an entity only through the cached handle, so `==` and table keys
  work; a dead entity or id -1 becomes nil. Every member of a handle checks
  the generation and raises "<kind> no longer exists".
- Adopt and release are idempotent: the pool event inside `Create*` adopts
  the entity before the binding that called `Create*` adopts it again.
- Players are released after the disconnect dispatch, entities after the
  "deleted" dispatch of `onEntityPoolChange`.

