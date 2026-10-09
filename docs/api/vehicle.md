# Vehicle

`Vehicle` is the class of vehicles. Scripts create vehicles with
`Vehicle.create`, and get handles to existing ones from events,
`Vehicle.findByID` and `Vehicle.getActive`.

See [entities](entities.md) for the members every entity has (`id`, `valid`,
`data`, `Vehicle.findByID`, `Vehicle.getActive`, `Vehicle.count`,
`tostring`, ...) and for how arguments are checked. The types used below
(integer, number, boolean, vector) are the same as in [Player](player.md).

## Lifetime

- The server owns vehicles. A vehicle stays until `vehicle:destroy()`, or
  until the server or another plugin deletes it.
- Dropping or garbage-collecting a handle deletes nothing. The vehicle can
  be found again with `Vehicle.findByID` or `Vehicle.getActive`.
- Once the vehicle is gone, its handle raises `vehicle no longer exists`;
  `vehicle.valid` is `false`. If the server reuses the id, the old handle
  still does not refer to the new vehicle.
- `Server.reload()` deletes the vehicles the scripts created.

## Creating vehicles

```text
Vehicle.create(model, world, x, y, z, angle[, colour1[, colour2]])
Vehicle.create(model, world, {x, y, z[, angle]}[, colour1[, colour2]])
```

- `model` and `world` are integers.
- In the first form `angle` is required. In the table form it is the
  optional fourth element, default `0`; a value that is not a number raises
  an error.
- `colour1` and `colour2` are integer colour ids. Each defaults to `-1`.
- Returns the new vehicle's handle.
- If the server refuses, it raises an error that names the reason, e.g.
  `'create' failed: pool exhausted`.

`Vehicle.new(...)` and `Vehicle(...)` take the same arguments and do the
same as `Vehicle.create(...)`.

## Return values and errors

- Methods that send a request to the server return `true`, or `false` when
  the server refuses it. Any other server error raises
  `'<name>' failed: <reason>`.
- Assigning a property raises the same errors. An assignment the server
  refuses does nothing.
- Assigning a read-only property raises an error.

## Static functions

These change handling rules for every vehicle of a model.

| Function | Returns | Notes |
| --- | --- | --- |
| `Vehicle.create(...)`, `Vehicle.new(...)`, `Vehicle(...)` | Vehicle | See [Creating vehicles](#creating-vehicles). |
| `Vehicle.getModelHandlingRule(model, rule)` | number | Raises if the server reports an error. |
| `Vehicle.setModelHandlingRule(model, rule, value)` | boolean | `value` is a number. |
| `Vehicle.modelHandlingRuleExists(model, rule)` | boolean | Whether the rule is set for the model. Raises if the server reports an error. |
| `Vehicle.resetModelHandlingRule(model, rule)` | boolean | |
| `Vehicle.resetModelHandlingRules(model)` | boolean | Resets every rule of the model. |
| `Vehicle.resetAllHandlings()` | nothing | Resets the handling of every model. |

`model` and `rule` are integers.

## Methods

### Lifetime and state

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:destroy()` | boolean | Deletes the vehicle. The handle no longer works afterwards. `false` if the server refused; the vehicle and its handle stay. |
| `vehicle:respawn()` | boolean | |
| `vehicle:explode()` | boolean | |
| `vehicle:repair()` | boolean | Sets health to 1000, clears the damage data and the low 8 bits of the lights data. `false` if the server refused any of these. |
| `vehicle:fix()` | boolean | Same as `repair`. |
| `vehicle:getModel()` | integer | Same as the `model` property. |

### Occupants and players

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:getOccupant(slot)` | Player or nil | The player in seat `slot` (integer), or `nil` for an empty seat. Raises if the server reports another error, e.g. `'getOccupant' failed: argument out of bounds` for an invalid slot. |
| `vehicle:streamedForPlayer(player)` | boolean | Whether the vehicle is streamed in for `player` (a live `Player`). |
| `vehicle:set3DArrowToPlayer(player, on)` | boolean | Turns the 3D arrow for `player` on or off; `on` is a boolean. |
| `vehicle:get3DArrowToPlayer(player)` | boolean | |

To put a player in a vehicle, use `player:setVehicle(vehicle[, slot])` or
`player.vehicle = vehicle`; see [Player](player.md).

### Options, parts and tyres

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:getOption(option)` | boolean | `option` is a `VehicleOption` value. Raises if the server reports an error. |
| `vehicle:setOption(option, on)` | boolean | `on` is a boolean. |
| `vehicle:getPartStatus(part)` | integer | Raises if the server reports an error. |
| `vehicle:setPartStatus(part, status)` | boolean | Both integers. |
| `vehicle:getTyreStatus(tyre)` | integer | Raises if the server reports an error. |
| `vehicle:setTyreStatus(tyre, status)` | boolean | Both integers. |

`VehicleOption` has `lockDoors`, `alarm`, `lights`, `radioLocked`, `ghost`,
`siren`, `singleUse`, `engine`, `boot` and `bonnet`.

### Speed

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:getSpeed([type])` | `{x, y, z}` | `type` is a `VehicleSpeed` value, default `VehicleSpeed.normal`. |
| `vehicle:setSpeed([type,] x, y, z[, add])` | boolean | Sets the speed of that `type`, default `VehicleSpeed.normal`. |
| `vehicle:setSpeed([type,] {x, y, z}[, add])` | boolean | The same with a table. |

- `VehicleSpeed` has `normal`, `normalRelative`, `turn` and
  `turnRelative`. The `turn` types read and set the turn speed.
- `add` is a boolean, default `false`. With `true` the vector is added to
  the current speed instead of replacing it.
- Any other `type` raises `bad argument #1 to '<method>' (VehicleSpeed
  value expected)`.

### Rotation

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:getRotation()` | table | `{euler = {x, y, z}, quaternion = {x, y, z, w}}`. Same as the `rotation` property. |
| `vehicle:setRotation(x, y, z)` | nothing | Euler angles. |
| `vehicle:setRotation(x, y, z, w)` | nothing | A quaternion. |
| `vehicle:setRotation(table)` | nothing | Any table form below. |
| `vehicle:getTurretRotation()` | number, number | Two values: horizontal, vertical. |

A rotation table, for `setRotation`, `rotation`, `angle` and
`spawnRotation`, is one of:

- `{x, y, z}`: Euler angles.
- `{x, y, z, w}`: a quaternion. Any table with four or more array elements
  is read as a quaternion.
- `{euler = {x, y, z}, quaternion = {x, y, z, w}}`, the table the getter
  returns. `quaternion` is used when present, else `euler`. So
  `vehicle.rotation = vehicle.rotation` works.

Tables are read without metamethods. A missing or non-number element raises
an error.

### Handling

These change the handling of this vehicle only. `rule` is an integer.

| Method | Returns | Notes |
| --- | --- | --- |
| `vehicle:getHandlingRule(rule)` | number | Raises if the server reports an error. |
| `vehicle:setHandlingRule(rule, value)` | boolean | `value` is a number. |
| `vehicle:hasHandlingRule(rule)` | boolean | Whether the rule is set for this vehicle. Raises if the server reports an error. |
| `vehicle:resetHandlingRule(rule)` | boolean | |
| `vehicle:resetHandlingRule({rule, ...})` | boolean | Resets each rule in the list. `true` only if every reset succeeded. |
| `vehicle:resetHandling()` | boolean | Resets every rule of this vehicle. |

## Properties

| Name | Type | Access | Notes |
| --- | --- | --- | --- |
| `model` | integer | read | |
| `wrecked` | boolean | read | |
| `world` | integer | read-write | |
| `health` | number | read-write | |
| `position` | `{x, y, z}` | read-write | Occupants stay in the vehicle when it moves. |
| `spawnPosition` | `{x, y, z}` | read-write | |
| `rotation` | table | read-write | Reads as `{euler = {x, y, z}, quaternion = {x, y, z, w}}`. Takes any rotation table (see [Rotation](#rotation)). |
| `angle` | table | read-write | Same as `rotation`. |
| `spawnRotation` | `{x, y, z}` | read-write | Reads as Euler angles. Takes any rotation table. |
| `color` | `{primary, secondary}` | read-write | Two integer colour ids. Assigning a table with only the first element, e.g. `{5}`, keeps the secondary colour; the same for `{nil, 5}` and the primary. A non-integer element raises an error. |
| `idleRespawnTime` | integer | read-write | Milliseconds, 0 to 4294967295. |
| `radio` | integer | read-write | |
| `damage` | integer | read-write | Damage data, 0 to 4294967295. |
| `lightsData` | integer | read-write | 0 to 4294967295. |
| `taxiLight` | boolean | read-write | Bit 8 (`0x100`) of `lightsData`. |
| `immunity` | integer | read-write | Flags, 0 to 4294967295. |

## Example

```lua
local spawned = {}

Event.bind("onPlayerCommand", function(player, command, args)
    if command ~= "car" then
        return
    end
    local old = spawned[player]
    if old and old.valid then
        old:destroy()
    end
    local model = args and tonumber(args[1]) or 141
    local x, y, z = table.unpack(player.position)
    local car = Vehicle.create(model, player.world, { x + 3, y, z, player.angle }, 1, 1)
    car:setOption(VehicleOption.lockDoors, false)
    spawned[player] = car
    player:setVehicle(car)
end)

Event.bind("onPlayerDisconnect", function(player)
    local car = spawned[player]
    if car and car.valid then
        car:destroy()
    end
    spawned[player] = nil
end)
```
