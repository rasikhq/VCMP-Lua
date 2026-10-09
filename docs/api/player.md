# Player

`Player` is the class of connected players. Scripts never create players:
handles come from events, `Player.findByID`, `Player.findByName` and
`Player.getActive`. A player's handle stops working when the player
disconnects.

See [entities](entities.md) for the members every entity has (`id`, `valid`,
`data`, `Player.findByID`, `Player.count`, `tostring`, ...) and for how
arguments are checked. Changes from 2.x are listed in the
[migration guide](../MIGRATION-v3.md).

Types used below:

- **integer**: a 32-bit signed integer unless the row gives another range. A
  float with an integral value, such as `1000 / 2`, is accepted.
- **number**: any Lua number.
- **boolean**: only `true` or `false`. Numbers are refused.
- **string**: a string; a number is converted to a string.
- **colour**: an RGBA colour as a 32-bit integer, e.g. `0xFF0000FF` for red.
  Values from -2147483648 to 4294967295 are accepted, so `-1` is the same as
  `0xFFFFFFFF`.
- **vector**: a table `{x, y, z}`, or three numbers in place of the table.
  Properties return vectors as `{x, y, z}` tables and take a table on
  assignment.

## Return values and errors

- Methods that send a request to the server return `true`, or `false` when
  the server refuses it (for example a request for a player who is not
  spawned). Any other server error raises `'<name>' failed: <reason>`, e.g.
  `'getOption' failed: argument out of bounds`.
- Assigning a property raises the same `'<name>' failed: <reason>` errors.
  An assignment the server refuses does nothing.
- Assigning a read-only property raises an error.
- A bad argument raises `bad argument #<n> to '<method>' (...)`, or
  `bad value for '<property>' (...)` for an assignment.

## Static functions

| Function | Returns | Notes |
| --- | --- | --- |
| `Player.getActive([spawnedOnly])` | table | `{[id] = player}` of every connected player. With `spawnedOnly` (boolean, default `false`) set to `true`, only spawned players. Replaces the plain `getActive()` every entity class has. |
| `Player.findByName(name)` | Player or nil | The connected player with that name, or `nil`. |
| `Player.msgAll(text[, colour])` | nothing | Sends `text` to every connected player. `colour` defaults to `0xFFFFFFFF` (white). |
| `Player.announceAll(text[, type])` | nothing | Shows `text` as a game message to every connected player. `type` is an integer, default `0`. |

`msgAll` and `announceAll` do not report server errors.

## Methods

### Messages

| Method | Returns | Notes |
| --- | --- | --- |
| `player:msg(text[, colour])` | boolean | Sends a chat message. `colour` defaults to `0xFFFFFFFF`. The text is sent as is: `%` has no special meaning. |
| `player:announce(text[, type])` | boolean | Shows a game message. `type` is an integer, default `0`. |

### Weapons and money

| Method | Returns | Notes |
| --- | --- | --- |
| `player:setWeapon(weapon, ammo)` | boolean | Both integers, both required. |
| `player:giveWeapon(weapon, ammo)` | boolean | Both integers, both required. |
| `player:removeWeapon(weapon)` | boolean | |
| `player:disarm()` | boolean | Removes every weapon. |
| `player:getWeaponAtSlot(slot)` | integer | The weapon id in that slot. Raises if the server reports an error, such as for an invalid slot. |
| `player:getAmmoAtSlot(slot)` | integer | The ammo in that slot. Raises if the server reports an error, such as for an invalid slot. |
| `player:giveMoney(amount)` | boolean | Adds `amount` (integer, may be negative) to the player's cash. |

The current weapon and its ammo are the read-only properties `weapon` and
`ammo`.

### Vehicles

| Method | Returns | Notes |
| --- | --- | --- |
| `player:setVehicle(vehicle[, slot])` | boolean | Puts the player in `vehicle` (a live `Vehicle`) at seat `slot` (integer, default `0`, the driver's seat). The request is sent with the server's makeRoom and warp flags on. |
| `player:eject()` | boolean | Removes the player from their vehicle. |

The `vehicle` property does the same by assignment.

### Camera

| Method | Returns | Notes |
| --- | --- | --- |
| `player:setCamera(position, lookAt)` | boolean | Two vectors: two tables or six numbers. |
| `player:restoreCamera()` | boolean | Gives the camera back to the player. |
| `player:interpolateCamLookAt(lookAt, ms)` | boolean | `lookAt` is a vector; `ms` an integer from 0 to 4294967295. |

### Spawning, health and state

| Method | Returns | Notes |
| --- | --- | --- |
| `player:forceSpawn()` | boolean | |
| `player:selectClass()` | boolean | Sends the player back to class selection. |
| `player:kill()` | boolean | |
| `player:addSpeed(speed)` | boolean | Adds a vector to the player's speed. |
| `player:setAnimation(animation)` | boolean | Plays an animation from group `0`. |
| `player:setAnimation(group, animation)` | boolean | Plays an animation from `group`. |
| `player:getAlpha()` | integer | |
| `player:setAlpha(alpha[, fadeTime])` | boolean | `alpha` is an integer; `fadeTime` an integer from 0 to 4294967295, default `0`. |
| `player:setDrunkHandling(level)` | boolean | `level` from 0 to 4294967295. Same as assigning `drunkHandling`. |
| `player:setDrunkVisuals(level)` | boolean | `level` from 0 to 255. Same as assigning `drunkVisuals`. |

### Options, streaming and 3D arrows

| Method | Returns | Notes |
| --- | --- | --- |
| `player:getOption(option)` | boolean | `option` is a `PlayerOption` value. Raises if the server reports an error, such as for an unknown option. |
| `player:setOption(option, on)` | boolean | `option` is a `PlayerOption` value; `on` a boolean. |
| `player:isPlayerStreamed(other)` | boolean | Whether this player is streamed in for `other` (a live `Player`). |
| `player:set3DArrowToPlayer(target, on)` | boolean | Turns the 3D arrow for `target` (a live `Player`) on or off; `on` is a boolean. |
| `player:get3DArrowToPlayer(target)` | boolean | |

`PlayerOption` has `controllable`, `driveBy`, `whiteScanLines`,
`greenScanLines`, `widescreen`, `showMarkers`, `canAttack`, `hasMarker`,
`showOnRadar` (same as `hasMarker`), `chatTags`, `drunkEffects` and
`bleeding`.

### Sound

| Method | Returns | Notes |
| --- | --- | --- |
| `player:playSound(sound)` | boolean | Plays a sound for this player only, not at a position. |
| `player:playSound3D(sound[, position])` | boolean | Plays a sound at `position` (a vector), or at the player's position when it is omitted. |

Both play the sound in the player's unique world.

### Connection

| Method | Returns | Notes |
| --- | --- | --- |
| `player:kick()` | nothing | Disconnects the player. `onPlayerDisconnect` runs before `kick` returns, and the handle no longer works afterwards. |
| `player:ban()` | nothing | Like `kick`, and bans the player. |
| `player:redirect(ip, port[, serverPassword[, userPassword]])` | boolean | Sends the player to another server under their current name. `port` is an integer from 0 to 4294967295; the passwords default to `""`. |
| `player:getModules()` | boolean | Asks the client for its module list. The list arrives later in `onPlayerModuleList(player, list)`. |
| `player:getNetworkStatistics(option)` | number | `option` is a `NetworkStatistics` value, e.g. `NetworkStatistics.packetLossTotal`. Raises if the server reports an error. |

### Getter methods

These methods return the same value as the read-only property next to them.

| Method | Property |
| --- | --- |
| `player:getIP()` | `ip` |
| `player:getUID()` | `uid` |
| `player:getUID2()` | `uid2` |
| `player:getKey()` | `key` |
| `player:getState()` | `state` |
| `player:getUniqueWorld()` | `uniqueWorld` |
| `player:getClass()` | `class` |
| `player:isOnline()` | `online` |
| `player:isSpawned()` | `spawned` |
| `player:isTyping()` | `typing` |
| `player:isCrouching()` | `crouching` |
| `player:isAway()` | `away` |
| `player:getPing()` | `ping` |
| `player:getFPS()` | `fps` |
| `player:getDrunkHandling()` | `drunkHandling` |
| `player:getDrunkVisuals()` | `drunkVisuals` |

## Properties

### Read-only

| Name | Type | Access | Notes |
| --- | --- | --- | --- |
| `ip` | string | read | Raises if the server cannot provide it. |
| `uid` | string | read | Raises if the server cannot provide it. |
| `uid2` | string | read | Raises if the server cannot provide it. |
| `key` | integer | read | |
| `state` | integer | read | A `PlayerState` value: `none`, `normal`, `aim`, `driver`, `passenger`, `enterDriver`, `enterPassenger`, `exit`, `unspawned`. |
| `uniqueWorld` | integer | read | |
| `class` | integer | read | The class the player chose. |
| `online` | boolean | read | |
| `spawned` | boolean | read | |
| `typing` | boolean | read | |
| `crouching` | boolean | read | |
| `away` | boolean | read | |
| `onFire` | boolean | read | |
| `cameraLocked` | boolean | read | |
| `ping` | integer | read | |
| `fps` | number | read | |
| `action` | integer | read | |
| `gameKeys` | integer | read | |
| `weapon` | integer | read | The current weapon. |
| `ammo` | integer | read | The current weapon's ammo. |
| `vehicleSlot` | integer | read | The seat the player is in. |
| `vehicleStatus` | integer | read | A `PlayerVehicle` value: `outside`, `entering`, `exiting`, `inside`. |
| `aimPosition` | `{x, y, z}` | read | |
| `aimDirection` | `{x, y, z}` | read | |
| `standingOnVehicle` | Vehicle or nil | read | |
| `standingOnObject` | Object or nil | read | |

### Read-write

| Name | Type | Access | Notes |
| --- | --- | --- | --- |
| `name` | string | read-write | Always read from the server. The server refuses invalid names: `'name' failed: invalid name`. |
| `admin` | boolean | read-write | |
| `health` | number | read-write | |
| `armour` | number | read-write | |
| `position` | `{x, y, z}` | read-write | |
| `speed` | `{x, y, z}` | read-write | |
| `angle` | number | read-write | The heading. |
| `world` | integer | read-write | |
| `secondaryWorld` | integer | read-write | |
| `team` | integer | read-write | |
| `skin` | integer | read-write | |
| `color` | colour | read-write | Reads as an integer from 0 to 4294967295. |
| `cash` | integer | read-write | |
| `score` | integer | read-write | |
| `wantedLevel` | integer | read-write | |
| `immunity` | integer | read-write | Flags, 0 to 4294967295. |
| `weaponSlot` | integer | read-write | |
| `drunkHandling` | integer | read-write | 0 to 4294967295. |
| `drunkVisuals` | integer | read-write | 0 to 255. |
| `vehicle` | Vehicle or nil | read-write | The vehicle the player is in, or `nil`. Assigning a vehicle puts the player in the driver's seat, like `setVehicle(vehicle)`. Assigning `nil` removes the player from their vehicle, and does nothing when they are on foot. |
| `spectateTarget` | Player or nil | read-write | The player being spectated, or `nil`. Assigning `nil` stops spectating. |

## Example

```lua
Event.bind("onPlayerCommand", function(player, command, args)
    if command == "heal" then
        if player.health >= 100 then
            player:msg("You are not hurt.", 0xFFFF00FF)
            return
        end
        player.health = 100
        player.armour = 100
        player:msg("Healed.", 0x00FF00FF)
    elseif command == "goto" and args then
        local target = Player.findByName(args[1])
        if target == nil then
            player:msg("No player called " .. args[1] .. ".", 0xFF0000FF)
            return
        end
        local x, y, z = table.unpack(target.position)
        player.world = target.world
        player.position = { x + 1, y, z }
    end
end)
```
