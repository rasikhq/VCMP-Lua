# Migrating from v1 to v2

Draft: each phase adds the changes it makes; phase 5 completes this guide.

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
  v1 accepted one of them.
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
- `Event.cancel()` applies to the innermost event being dispatched. In v1
  one global flag was shared by nested events.
- `Event.trigger` returns false when a handler cancelled the event (v1
  always returned true).
- `onServerInit` also runs after `Server.reload()`.
- At server shutdown, `onPlayerDisconnect` runs for every player still
  online, with reason 0, after `onServerShutdown`.
- `onPlayerWasted` runs every handler (v1 ran only the first one).
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

In v1 the Lua garbage collector owned these entities: an entity whose Lua
object was not stored somewhere disappeared at the next collection, and
handles from `findByID`, `getActive` or events then pointed at freed memory.
In v2 the server owns them, as in Squirrel:

- `Vehicle.create(...)` creates a vehicle and returns its handle. The v1
  forms `Vehicle(...)` and `Vehicle.new(...)` still work and do the same.
  The same goes for `Object`, `Pickup` and `Checkpoint`.
- The entity stays until `entity:destroy()`, or until the server or
  another plugin deletes it. Dropping the handle deletes nothing.
- `Server.reload()` deletes the entities the scripts created.

## Player

- The members keep their v1 names and arguments.
- `ammo` is the ammo of the current weapon (v1's always raised an error).
- `name` is always the server's current name (v1 cached it and could
  return an old one); a name the server refuses raises an error.
- `Player.getActive(true)` returns only spawned players (v1 ignored the
  argument).
- `player.vehicle = nil` removes the player from the vehicle and
  `player.spectateTarget = nil` stops spectating (both crashed in v1).
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

- Creation: see "Lifetime" above. The arguments are v1's:
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

- Creation: see "Lifetime" above; v1's arguments
  `(model, world, x, y, z[, alpha])` or `(model, world, {x, y, z[, alpha]})`.
- `object.angle = {x, y, z}` rotates the object to that angle (v1 rotated
  it by that angle).
- `rotateTo`/`rotateBy` use a quaternion when the table has four elements.
  v1 used Euler angles when the fourth element was -1.
- New: `model` and `alpha` properties.

## Pickup

- Creation: see "Lifetime" above; v1's arguments
  `(model, world, quantity, x, y, z, alpha, automatic)` or
  `(model, world, quantity, {x, y, z}, alpha, automatic)`. `alpha`
  (default 255) and `automatic` (default `true`) are optional now.
- New: `model` and `quantity` properties.

## Checkpoint

- Creation: see "Lifetime" above; v1's arguments
  `(player, world, isSphere, x, y, z, {r, g, b[, a]}, radius)` or
  `(player, world, isSphere, {x, y, z}, {r, g, b[, a]}, radius)`;
  `player` may be `nil` for a checkpoint every player sees.
- `checkpoint.radius = r` sets the radius (v1 compared `r` with the world
  first and often did nothing).
- Colour channels must be integers in [0, 255].
- New: `owner` and `sphere` properties.

## Bind (key binds)

- `Bind.create(signalOnRelease, key1[, key2[, key3]])` returns a handle;
  v1's `Bind(...)` and `Bind.new(...)` still work. The bind stays until
  `bind:destroy()`: dropping the handle removes nothing.
- `Bind.clearAllBinds()` removes only the binds the scripts created. v1
  removed every plugin's binds.
- A bind another plugin removed raises "bind no longer exists" on use.
- `onPlayerKeyDown`/`onPlayerKeyUp` pass the bind's handle also for binds
  that another plugin registered (v1 passed `nil`).
- `Server.reload()` removes the binds the scripts created.

## Stream

- Same members and byte layout as v1. `Stream()` and `Stream.new()`
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
  client. In v1 more than 4096 bytes overflowed a buffer on the server's
  stack.
- New: `stream.size` and `stream.remaining`.

## Server, Map, Radio, Weapon, Blip, Sound

- `Server` keeps v1's functions and properties (`Server.name`,
  `Server.hour = 12`, ...).
- `Server.addClass` reads the optional weapons from the right arguments
  (v1 was off by one) and accepts all three forms:
  `(team, colour, skin, x, y, z, angle, ...)`,
  `(team, colour, skin, {x, y, z}, angle, ...)` and
  `(team, colour, skin, {x, y, z, angle}, ...)`. It returns the class id.
- `Server.banIP`, `unbanIP` and `isIPBanned` accept strings (v1 refused
  them).
- `Server.getSkinName(id)` returns `nil` for an unknown id (v1 crashed).
- `Server.createExplosion(world, type, position[, creator[, atGroundLevel]])`:
  the last two arguments are optional.
- `Server.wastedSettings = {...}` keeps the fields the table leaves out.
- `Map.hideObject`/`showMapObject` round coordinates to the nearest tenth;
  v1 got negative coordinates wrong by one tenth (-2.0 became -1.9).
  `Map.showObject` is a new alias of `showMapObject`.
- `Radio.createStream([id,] name, url[, listed])` returns `true` or
  `false` in both forms (v1 returned an error code number when an id was
  given); `listed` defaults to `true`.
- `Weapon.resetAll()` takes no argument.
- `Blip.create(...)` returns a `Blip` handle instead of a number, with the
  same arguments as v1. `Blip.destroy` and `Blip.getInfo` accept the handle
  or a blip id, and the handle has `blip:destroy()`, `blip:getInfo()`,
  `blip.id` and `blip.valid`.
- `Sound.play` accepts every v1 form; the position may be a table or three
  numbers.
- `onServerPerformanceReport(count, descriptions, times)` passes two
  arrays (v1 passed raw C pointers).
- New event: `onEntityStreamingChange(player, entityType, entityId,
  deleted)` (plugin API 2.1).

## Logger

- `Logger.debug/info/warn/error/critical(message)` keep their names.
- `Logger.setLevel(level)` sets the least severe level that is logged:
  `"debug"`, `"info"`, `"warn"`, `"error"`, `"critical"`, `"off"`, or v1's
  numbers 0 (debug) to 4 (critical), 5 for off. In v1 the numbers worked
  backwards and level 0 still logged everything. The setting is the same
  one as `log.level` in `luaconfig.lua`; a reload restores the config's.
- New: `Logger.getLevel()`.

## Removed

These v1 globals are gone. Using one raises an error that names its
replacement, e.g. "MySQL was removed in v2: use require "luasql.mysql"
(see docs/MIGRATION-v2.md#mysql)". The replacement libraries arrive in
phase 4.

### MySQL

`MySQL.createConnection` and its connection objects ran queries on worker
threads, which crashed the server (plan A1). Use LuaSQL:
`require "luasql.mysql"` (MariaDB Connector/C, linked into the plugin).
Queries run synchronously on the server thread.

### SQLite

`SQLite`, `SqLite` and `SQLiteDatabase` are replaced by
`require "luasql.sqlite3"`.

### Remote

`Remote` (HTTP over cpr) is replaced by the `http` module:
`require "http"`, `http.request{url, method, headers, body, timeout}`
with a callback. Requests never block the server.

### JSON

The embedded JSON library is replaced by lua-cjson: `require "cjson"`.

### Thread

`Thread` ran Lua on worker threads, which a Lua state does not allow
(plan A1). There is no replacement: scripts run on the server's thread.
Use timers, and the non-blocking `http` module.

### Debugger

`dbg` stopped the whole server while it waited for console input. It has
no replacement.

## New

- `Server.reload()` reloads `luaconfig.lua` and every script.
