# Internals

Facts about the VC:MP server that the v2 runtime is built on, measured in
phase 1. They settle the entity design (plan B4) and the shutdown order
(plan B3.1).

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

## Plugin API structs

| Struct | Size reported by the server | Size in `third_party/vcmp/vcmp.h` |
|---|---|---|
| `PluginFuncs` | 2360 bytes | 2352 bytes |
| `PluginCallbacks` | 376 bytes | 368 bytes |
| `PluginInfo` | 48 bytes | 48 bytes |

The server has one more function and one more callback than our SDK header.
Fields are only appended, so the header still matches as far as it goes, and
the `structSize` checks (plan B3.5) handle both directions. Finding the newer
header is a phase 3 task.

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

## Pending: players

Kicks, disconnects and a shutdown with a player online need a real game
client (VC:MP does not support NPCs). Still to measure:

- whether `OnPlayerDisconnect` fires inside `KickPlayer`, and its reason code;
- what `IsPlayerConnected` and `GetPlayerName` return during
  `OnPlayerDisconnect`;
- the reason code when a player quits;
- whether `OnPlayerDisconnect` comes before or after `OnServerShutdown`
  when the server stops with a player online.

To run it: build `tests/probe` (see its CMakeLists.txt), put `mpsvrrel64`,
`server.cfg` (`port 5192`, `plugins probe_a probe_b`) and `plugins/` in one
directory, start the server with `-p 5192:5192/udp`, then join three times:
join 1 is kicked after 3 s, quit join 2 yourself, and 5 s after join 3 the
probe calls `ShutdownServer()`.

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
