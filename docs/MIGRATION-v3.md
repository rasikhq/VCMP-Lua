# Migrating from 2.x to 3.0

3.0 rebuilds the plugin around one rule: a script can not crash the server.
Most scripts written for 2.x (the 2.0 to 2.8 beta releases) keep working;
this guide lists every change. The full API is in
[docs/api](api/).

## Installing

- The plugin is one file per platform: `plugins/LuaPlugin_x64.dll`
  (Windows) or `plugins/LuaPlugin_x64.so` (Linux, glibc 2.28 or newer). It
  needs no other files and no VC++ redistributable. The DLLs and `.so`
  files 2.x needed (OpenSSL, MySQL, LuaSocket, Lanes) can go.
- Only 64-bit servers are supported.

## Configuration: luaconfig.ini becomes luaconfig.lua

`luaconfig.ini` is no longer read. Put `luaconfig.lua` in the same place;
it returns a table ([configuration](configuration.md) lists every field):

```ini
[config]
loglevel=0
logfile=DailyLogs.logs

[modules]
lanes=false

[scripts]
script=lua/script.lua
```

becomes

```lua
return {
  scripts = { "lua/script.lua" },
  log = { level = "debug", file = "DailyLogs.logs", daily = true },
}
```

- `loglevel` becomes `log.level`, a name: `"trace"`, `"debug"`, `"info"`,
  `"warn"`, `"error"`, `"critical"` or `"off"`.
- `logfile` becomes `log.file`; `daily = true` starts a new file every day,
  as 2.x did.
- `[modules]` is gone: every library is built in and loads with `require`.
- `experimental` is gone, and so are `__experimental__` and
  `__reload_scripts`. Use `Server.reload()`.
- A mistake in the file keeps the plugin from starting, with a message
  such as `luaconfig.lua: scripts[2] must be a string (got number)`. During
  `Server.reload()` the old scripts keep running instead. Unknown settings
  are logged and ignored.
- Scripts run when the server has started (`onServerInit`), not while the
  plugin loads.

## Errors

Bindings check their arguments and raise errors instead of logging and
carrying on, or crashing. The messages read like Lua's own and carry the
script position:

```
main.lua:12: bad argument #1 to 'setWeapon' (integer expected, got string)
main.lua:13: bad value for 'health' (number expected, got string)
main.lua:14: bad argument #2 to 'create' (value 5000000000 out of range [-2147483648, 2147483647])
```

- Integers must be integral: `1000 / 2` (500.0) is accepted, `1.5` is not,
  and a value that does not fit the server's type is refused instead of
  being cut down.
- Booleans must be `true` or `false`; `0` is not false.
- Colours accept 32-bit values written as unsigned (`0xFF0000FF`) or as
  signed integers (`-16776961`).
- Positions and vectors are a table `{x, y, z}` or three numbers, wherever
  2.x accepted one of them.
- When the server refuses a request (for example `setVehicle` for a player
  who is not spawned), the method returns `false`. Any other error the
  server reports raises "'name' failed: argument out of bounds" and the
  like. Getters return booleans as `true`/`false`, never `0`/`1`.

## Events

- `Event.bind`, `Event.unbind`, `Event.create`, `Event.cancel` and
  `Event.trigger` keep their names and arguments.
- Mistakes raise errors instead of logging and returning false: an unknown
  event name, a handler that is not a function, `Event.cancel()` outside an
  event handler. Binding a function twice and unbinding a function that is
  not bound still return false.
- `Event.cancel()` applies to the innermost event being dispatched. In 2.x
  one global flag was shared by nested events.
- `Event.trigger` returns false when a handler cancelled the event (2.x
  always returned true).
- `onServerInit` also runs after `Server.reload()`.
- At server shutdown, `onPlayerDisconnect` runs for every player still
  online, with reason 0, after `onServerShutdown`.
- `onPlayerWasted` runs every handler (2.x ran only the first one).
- `onPlayerFireChange`, `onPlayerCrouchChange` and `onPlayerAwayChange`
  pass `true`/`false` instead of `1`/`0`.
- `onPlayerCommand(player, command, args, text)` has a fourth argument: the
  text after the command, with its spaces (`"/pm Bob hi there"` gives
  `"Bob hi there"`).
- An event about an entity the scripts have not seen yet (for example one
  that another plugin created) passes its handle instead of `nil`.

## Timers

- `Timer.create(fn, interval, repeats, ...)` and `Timer.destroy(timer)` keep
  their names; `timer:destroy()` and `timer.active` are new.
- The 50 ms minimum interval is gone (0 runs the timer every frame).
  Invalid arguments raise errors instead of returning nil.
- Timers no longer stop after about 71.6 minutes of uptime.

## Entities

- A script holds a handle; the same entity is always the same handle, so
  `==` and table keys work. Using a handle after its entity is gone raises
  "<kind> no longer exists" instead of crashing or touching another entity.
- `handle.valid` is new: `false` once the entity is gone, without raising.

## Lifetime of vehicles, objects, pickups and checkpoints

In 2.x the Lua garbage collector owned these entities: an entity whose Lua
object was not stored somewhere disappeared at the next collection, and
handles from `findByID`, `getActive` or events then pointed at freed memory.
In 3.0 the server owns them, as in Squirrel:

- `Vehicle.create(...)` creates a vehicle and returns its handle. The 2.x
  forms `Vehicle(...)` and `Vehicle.new(...)` still work and do the same.
  The same goes for `Object`, `Pickup` and `Checkpoint`.
- The entity stays until `entity:destroy()`, or until the server or
  another plugin deletes it. Dropping the handle deletes nothing.
- `Server.reload()` deletes the entities the scripts created.

## Player

- The members keep their 2.x names and arguments.
- `ammo` is the ammo of the current weapon (2.x's always raised an error).
- `name` is always the server's current name (2.x cached it and could
  return an old one); a name the server refuses raises an error.
- `Player.getActive(true)` returns only spawned players (2.x ignored the
  argument).
- `player.vehicle = nil` removes the player from the vehicle and
  `player.spectateTarget = nil` stops spectating (both crashed in 2.x).
- `ip`, `uid` and `uid2` raise an error if the server cannot provide them.
- `setAlpha(alpha[, fadeTime])` and `redirect(ip, port[, serverPassword[,
  userPassword]])`: the last arguments are optional.
- Methods that only send a request to the server (`setVehicle`,
  `forceSpawn`, `setCamera`, `kill`, ...) return `true`, or `false` when
  the server refuses.
- New: `Player.findByName(name)`, `speed`, `addSpeed(v)`, `away`, `onFire`,
  `gameKeys`, `standingOnVehicle`, `standingOnObject`, `giveMoney(amount)`,
  `getWeaponAtSlot(slot)`, `getAmmoAtSlot(slot)` and
  `getNetworkStatistics(NetworkStatistics.x)`.

## Vehicle

- Creation: see "Lifetime" above. The arguments are 2.x's:
  `(model, world, x, y, z, angle[, colour1[, colour2]])` or
  `(model, world, {x, y, z[, angle]}[, colour1[, colour2]])`.
- `vehicle.rotation = vehicle.rotation` works: the setter accepts the
  getter's `{euler = ..., quaternion = ...}` table as well as `{x, y, z}`
  and `{x, y, z, w}`.
- `getOccupant(slot)` returns `nil` for an empty seat and raises an error
  for an invalid slot.
- `setSpeed`: the final `add` argument is optional (default `false`).
- New: `model`, `wrecked`, `lightsData`, `explode()`.

## Object

- Creation: see "Lifetime" above; 2.x's arguments
  `(model, world, x, y, z[, alpha])` or `(model, world, {x, y, z[, alpha]})`.
- `object.angle = {x, y, z}` rotates the object to that angle (2.x rotated
  it by that angle).
- `rotateTo`/`rotateBy` use a quaternion when the table has four elements.
  2.x used Euler angles when the fourth element was -1.
- New: `model` and `alpha` properties.

## Pickup

- Creation: see "Lifetime" above; 2.x's arguments
  `(model, world, quantity, x, y, z, alpha, automatic)` or
  `(model, world, quantity, {x, y, z}, alpha, automatic)`. `alpha`
  (default 255) and `automatic` (default `true`) are optional now.
- New: `model` and `quantity` properties.

## Checkpoint

- Creation: see "Lifetime" above; 2.x's arguments
  `(player, world, isSphere, x, y, z, {r, g, b[, a]}, radius)` or
  `(player, world, isSphere, {x, y, z}, {r, g, b[, a]}, radius)`;
  `player` may be `nil` for a checkpoint every player sees.
- `checkpoint.radius = r` sets the radius (2.x compared `r` with the world
  first and often did nothing).
- Colour channels must be integers in [0, 255].
- New: `owner` and `sphere` properties.

## Bind (key binds)

- `Bind.create(signalOnRelease, key1[, key2[, key3]])` returns a handle;
  2.x's `Bind(...)` and `Bind.new(...)` still work. The bind stays until
  `bind:destroy()`: dropping the handle removes nothing.
- `Bind.clearAllBinds()` removes only the binds the scripts created. 2.x
  removed every plugin's binds.
- A bind another plugin removed raises "bind no longer exists" on use.
- `onPlayerKeyDown`/`onPlayerKeyUp` pass the bind's handle also for binds
  that another plugin registered (2.x passed `nil`).
- `Server.reload()` removes the binds the scripts created.

## Stream

- Same members and byte layout as 2.x. `Stream()` and `Stream.new()`
  create one.
- Reading past the end raises "Stream: not enough data to read ..."
  instead of returning 0 or an empty string; writing past 4096 bytes
  raises "Stream: no room to write ...". A failed read or write changes
  nothing.
- `writeByte` takes -128 to 255; `writeNumber` takes any 32-bit integer,
  signed or unsigned.
- `stream:send()` without a player sends to every player, as
  `stream:send(nil)` did.
- `onClientData(player, stream, size)` accepts data of any size from the
  client. In 2.x more than 4096 bytes overflowed a buffer on the server's
  stack.
- New: `stream.size` and `stream.remaining`.

## Server, Map, Radio, Weapon, Blip, Sound

- `Server` keeps 2.x's functions and properties (`Server.name`,
  `Server.hour = 12`, ...).
- `Server.addClass` reads the optional weapons from the right arguments
  (2.x was off by one) and accepts all three forms:
  `(team, colour, skin, x, y, z, angle, ...)`,
  `(team, colour, skin, {x, y, z}, angle, ...)` and
  `(team, colour, skin, {x, y, z, angle}, ...)`. It returns the class id.
- `Server.banIP`, `unbanIP` and `isIPBanned` accept strings (2.x refused
  them).
- `Server.getSkinName(id)` returns `nil` for an unknown id (2.x crashed).
- `Server.createExplosion(world, type, position[, creator[, atGroundLevel]])`:
  the last two arguments are optional.
- `Server.wastedSettings = {...}` keeps the fields the table leaves out.
- `Map.hideObject`/`showMapObject` round coordinates to the nearest tenth;
  2.x got negative coordinates wrong by one tenth (-2.0 became -1.9).
  `Map.showObject` is a new alias of `showMapObject`.
- `Radio.createStream([id,] name, url[, listed])` returns `true` or
  `false` in both forms (2.x returned an error code number when an id was
  given); `listed` defaults to `true`.
- `Weapon.resetAll()` takes no argument.
- `Blip.create(...)` returns a `Blip` handle instead of a number, with the
  same arguments as 2.x. `Blip.destroy` and `Blip.getInfo` accept the handle
  or a blip id, and the handle has `blip:destroy()`, `blip:getInfo()`,
  `blip.id` and `blip.valid`.
- `Sound.play` accepts every 2.x form; the position may be a table or three
  numbers.
- `onServerPerformanceReport(count, descriptions, times)` passes two
  arrays (2.x passed raw C pointers).
- New event: `onEntityStreamingChange(player, entityType, entityId,
  deleted)` (plugin API 2.1).

## Logger

- `Logger.debug/info/warn/error/critical(message)` keep their names.
- `Logger.setLevel(level)` sets the least severe level that is logged:
  `"debug"`, `"info"`, `"warn"`, `"error"`, `"critical"`, `"off"`, or 2.x's
  numbers 0 (debug) to 4 (critical), 5 for off. In 2.x the numbers worked
  backwards and level 0 still logged everything. The setting is the same
  one as `log.level` in `luaconfig.lua`; a reload restores the config's.
- New: `Logger.getLevel()`.

## Removed

These 2.x globals are gone. Using one raises an error that names its
replacement, e.g. "MySQL was removed in 3.0: use require "luasql.mysql"
(see docs/MIGRATION-v3.md#mysql)".

### MySQL

`MySQL.createConnection` and its connection objects ran queries on worker
threads, which crashed the server. Use LuaSQL:
`require "luasql.mysql"` (MariaDB Connector/C, linked into the plugin).
Queries run synchronously on the server thread; non-blocking queries will
follow once LuaSQL releases its asynchronous API. LuaSQL has no prepared
statements; build queries with `sql.format` (see "SQL values" below)
instead of concatenating values. 2.x bound every number as a float, so
integers above 2^24 were corrupted; `sql.format` writes integers exactly.

```lua
local env = require("luasql.mysql").mysql()
local conn = assert(env:connect("db", "user", "password", "127.0.0.1", 3306, nil, nil,
  { connect_timeout = 5, read_timeout = 30, write_timeout = 30 }))
```

The last argument (options) is VCMP-Lua's: `connect_timeout`,
`read_timeout`, `write_timeout` in seconds, `ssl` (`"disable"`,
`"require"` or `"verify"`) and `ssl_ca`. Postgres takes a connection
string: `require("luasql.postgres").postgres():connect("host=... dbname=...
user=... password=... connect_timeout=5")`. Postgres and MySQL return every
value as a string.

### SQLite

`SQLite`, `SqLite` and `SQLiteDatabase` are replaced by
`require "luasql.sqlite3"`.

### Remote

`Remote` (HTTP over cpr) is replaced by the `http` module:
`require "http"`, `http.request{url, method, headers, body, timeout}`
with a callback. Requests never block the server: the callback runs in a
later server frame.

```lua
local http = require "http"
http.request({
  url = "https://example.com/api",
  method = "POST",                                   -- default: GET, or POST with a body
  headers = { ["Content-Type"] = "application/json" },
  body = cjson.encode({ name = player.name }),
  timeout = 10,                                      -- seconds, default 30
  redirects = 5,                                     -- default 5; 0 does not follow
}, function(res, err)
  if not res then
    print("request failed: " .. err)
    return
  end
  print(res.status, res.headers["content-type"], res.body, res.url)
end)

http.request("https://example.com/", function(res, err) end)  -- a GET
```

- `res.headers` has lower-case names; repeated headers are joined with
  `", "`.
- Certificates and host names are always checked. To trust a private CA,
  set `http = { cafile = "ca.pem" }` in `luaconfig.lua`.
- Only `http://` and `https://` URLs are accepted. A response body may be
  at most 16 MiB, and at most 1000 requests may be pending.
- Requests still pending at `Server.reload()` or shutdown are cancelled;
  their callbacks do not run.

### JSON

The embedded JSON library is replaced by lua-cjson: `require "cjson"`
(`cjson.encode`, `cjson.decode`; `require "cjson.safe"` returns `nil, err`
instead of raising).

### Thread

`Thread` ran Lua on worker threads, which a Lua state does not allow.
There is no replacement: scripts run on the server's thread.
Use timers, and the non-blocking `http` module.

### Lanes

The `lanes` module ran Lua in other threads, and it is gone for the same
reason as `Thread`. Copas (`require "copas"`) runs coroutines on the server
thread, for scripts that wait on sockets.

### Debugger

`dbg` stopped the whole server while it waited for console input. It has
no replacement.

## Hash

The global `Hash` keeps 2.x's functions, with the same results, so stored
hashes still verify: `MD5`, `SHA1`, `SHA256`, `SHA512`, `Whirlpool`
(`Hash.SHA256(text)`) and the keyed `KMAC256`, `SKEIN256`, `SKEIN512`
(`Hash.KMAC256(key, text)`). All return lower-case hex. `require "hash"`
returns the same table.

New, for storing passwords properly (a plain or salted SHA digest is far
too fast to resist guessing):

- `Hash.pbkdf2(password, salt, iterations, length[, digest])`: hex of
  `length` bytes; `digest` defaults to `"sha256"`.
- `Hash.scrypt(password, salt, N, r, p, length)`: hex; `N` is a power of
  two, and `N * r * 128` bytes must fit in 32 MiB.
- `Hash.hmac(digest, key, data)`: hex.
- `Hash.randomBytes(n)`: `n` random bytes (binary) for salts and tokens;
  `Hash.toHex(bytes)` turns them into hex.
- `Hash.equals(a, b)`: compares two hashes in constant time.

Digests for `hmac` and `pbkdf2`: `md5`, `sha1`, `sha224`, `sha256`,
`sha384`, `sha512`, `sha3-256`, `sha3-512`.

## SQL values

`require "sql"` provides `sql.format(conn, text, ...)`: each `?` in the SQL
text takes the next value, escaped for that connection by `conn:escape`.

```lua
local sql = require "sql"
conn:execute(sql.format(conn, "INSERT INTO users (name, score) VALUES (?, ?)", name, 42))
```

`nil` becomes `NULL`, booleans `TRUE`/`FALSE`, numbers their exact value,
strings a quoted literal. `??` is a literal `?`, and a `?` inside a quoted
string, a quoted name or a comment stays. A wrong number of values, or a
value such as a table or NaN, raises an error.

## Modules

Every library is built into the plugin and loads with `require`, without
files on disk: `lfs`, `cjson`, `socket`, `copas`, `luasql.*`, `inspect`,
`http`, `hash` and `sql`. Lua modules on `package_path` (from
`luaconfig.lua`) load as before. See [modules](api/modules.md) for the list
and the limits:

- C modules cannot be loaded from disk.
- `load`, `loadfile`, `dofile` and `require` accept Lua source only, not
  precompiled bytecode.
- `loadfile()` and `dofile()` without a file name raise an error instead of
  waiting for console input.
- Never call `copas.loop()`: the plugin runs Copas every frame.

## New

- `Server.reload()` reloads `luaconfig.lua` and every script.
- Entity handles have `valid`. Their `data` table is dropped with the
  entity, so a new entity with the same id starts with an empty one.
- The `http` module, `sql.format`, and `Hash.pbkdf2`, `scrypt`, `hmac` and
  `randomBytes`.
- `Logger.getLevel()`.
