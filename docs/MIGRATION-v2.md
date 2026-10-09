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

## New

- `Server.reload()` reloads `luaconfig.lua` and every script.
