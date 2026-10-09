# Entities

Players, vehicles, objects, pickups, checkpoints, blips and key binds are
entities. A script never holds an entity itself: it holds a handle, a small
value that names one entity of one class. This page lists the members every
entity class has, and the argument rules every binding follows.

The entity classes are `Player`, `Vehicle`, `Object`, `Pickup`,
`Checkpoint`, `Blip` and `Bind`. Changes from 2.x are listed in
[MIGRATION-v3.md](../MIGRATION-v3.md).

## Handles

- Bindings and events return handles. You cannot build one yourself.
- The same entity is always the same handle. `==` and `rawequal` work, and
  a handle can be a table key.
- A handle belongs to one lifetime of its entity. When the entity goes, the
  handle is dead for good. If the server later reuses the id, the new
  entity gets a new handle, and the old one does not equal it.
- Every member of a dead handle raises `<kind> no longer exists`, for
  example `vehicle no longer exists`. Only `valid` and `tostring` do not
  raise.
- An event about an entity the scripts have not seen yet, such as one
  another plugin created, still passes its handle.

Ids run from the server's first id up to its pool size:

| Class | Ids |
|---|---|
| Player | 0 to 99 |
| Vehicle | 1 to 1000 |
| Object | 0 to 2999 |
| Pickup | 0 to 1999 |
| Checkpoint | 0 to 1999 |
| Blip | 0 to 99 |
| Bind | 0 to 255 |

## Lifetime

- The server owns entities, not Lua. Dropping or collecting a handle
  deletes nothing.
- An entity stays until `destroy()` is called, or until the server or
  another plugin deletes it. `Vehicle`, `Object`, `Pickup`, `Checkpoint`,
  `Blip` and `Bind` have `destroy()`. A player leaves by disconnecting.
- After a successful `destroy()` the handle is dead.
- `Server.reload()` deletes the entities and key binds the scripts
  created. Entities created by other plugins stay.

## Static functions

Call these on the class, for example `Vehicle.count()`.

### Kind.findByID(id)

Returns the handle of the entity with this id, or `nil` if none exists.
`id` must be a 32-bit integer. An id outside the pool, or `-1`, gives
`nil`.

### Kind.getActive()

Returns a table `{[id] = handle}` of every entity of this class that
exists, including those other plugins created. It is keyed by id, not a
sequence: iterate it with `pairs`. `Player.getActive` also takes an
optional `spawnedOnly` boolean.

### Kind.count()

Returns how many entities of this class exist.

### Kind.type()

Returns the class name, for example `"Vehicle"`.

## Methods

### handle:getID()

Returns the id. Same as `handle.id`.

### handle:getType()

Returns the class name, for example `"Object"`.

### tostring(handle)

Returns `"Vehicle(3)"`, or `"Vehicle(3, no longer exists)"` for a dead
handle. Never raises.

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `id` | integer | read | The server's id. |
| `valid` | boolean | read | `false` once the entity is gone. Never raises. |
| `data` | table | read/write | A table for your own values. Created on first read and dropped with the entity. Assign a table to replace it. |

A key bind is checked against the server when a member uses it, because
the server reports no event when another plugin removes one. Until then,
`bind.valid` can still be `true` for a bind that is gone.

## Argument conventions

These rules apply to every binding, not only entity members.

- **Positions and vectors** are a table `{x, y, z}` or three numbers. The
  table is read by index (`[1]`, `[2]`, `[3]`), not by `x`/`y`/`z` keys.
  A getter returns a new table `{x, y, z}` with the same layout.
- **Integers** must have an integral value: `1000 / 2` (500.0) is accepted,
  `1.5` is not. A value that does not fit the server's type is refused,
  never cut down.
- **Colours** are 32-bit values, written unsigned (`0xFF0000FF`) or signed
  (`-16776961`). Any value from -2147483648 to 4294967295 is accepted.
- **Booleans** must be `true` or `false`. `0`, `1` and `nil` are refused.
  Getters return `true`/`false`, never `0`/`1`.
- **Strings** accept numbers too, converted as Lua does.
- **Entity arguments** must be a live handle of the right class. An
  optional entity argument also accepts `nil`.
- **"None"**: where the server reports entity id `-1`, the binding returns
  `nil`.
- **Argument numbers** in errors do not count `self` when a method is
  called with `:`, as in Lua.

## Errors

Mistakes raise Lua errors. The messages read like Lua's own and start with
the script position:

```
main.lua:12: bad argument #1 to 'setWeapon' (integer expected, got string)
main.lua:13: bad value for 'health' (number expected, got string)
main.lua:14: bad argument #2 to 'create' (value 5000000000 out of range [-2147483648, 2147483647])
main.lua:15: bad argument #1 to 'moveTo' (number has no integer representation)
main.lua:16: bad argument #1 to 'streamedForPlayer' (Player expected, got Vehicle)
main.lua:17: vehicle no longer exists
```

A property assignment says `bad value for '<property>'` instead of
`bad argument`. An element of a table argument is named in the message,
for example `(element 3 must be a number, got nil)`.

Methods that send a request to the server return `true`, or `false` when
the server refuses it (for example, a request about a player who is not
spawned). A property assignment ignores a refusal. Any other error the
server reports raises `'<function>' failed: <reason>`, where the reason is
one of `no such entity`, `buffer too small`, `input too large`,
`argument out of bounds`, `null argument`, `pool exhausted`,
`invalid name` or `unknown error`. A `create` function the server refuses
raises `'create' failed: ...`; it never returns `nil`.

A server function the running server does not have raises
`<function> is not supported by this server version`. A binding called
while the scripts are shutting down raises `runtime shutting down`.

## Example

```lua
local score = {}

Event.bind("onPlayerConnect", function(player)
    player.data.joined = os.time()
    score[player] = 0
end)

Event.bind("onPlayerDisconnect", function(player, reason)
    -- The handle is still valid during this event.
    local joined = player.data.joined or os.time()
    print(tostring(player) .. " played " .. (os.time() - joined) .. " s")
    score[player] = nil
end)

Event.bind("onPlayerCommand", function(player, command, args, text)
    if command == "players" then
        for id, other in pairs(Player.getActive()) do
            player:msg(id .. ": " .. other.name .. " (" .. (score[other] or 0) .. ")")
        end
    end
end)
```
