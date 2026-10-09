# Enums

The plugin registers these constant tables as globals. Each value is an
integer, and you pass it wherever a function or event uses that kind of
value, for example `Server.setOption(ServerOption.friendlyFire, true)`.

The tables are read-only: assigning a key raises
`sol: cannot modify the elements of an enumeration table`. They cannot be
iterated with `pairs`, which yields nothing; use the keys listed here.

## ServerOption

Used by `Server.getOption` and `Server.setOption`.

| Key | Value | Key | Value |
|---|---|---|---|
| `syncFrameLimiter` | 0 | `showMarkers` | 10 |
| `frameLimit` | 1 | `teamMarkersOnly` | 11 |
| `taxiBoostJump` | 2 | `stuntBike` | 12 |
| `driveOnWater` | 3 | `shootInAir` | 13 |
| `fastSwitch` | 4 | `nametags` | 14 |
| `friendlyFire` | 5 | `joinMessages` | 15 |
| `driveBy` | 6 | `deathMessages` | 16 |
| `perfectHandling` | 7 | `chatTags` | 17 |
| `flyingCars` | 8 | `classes` | 18 |
| `jumpSwitch` | 9 | `wallGlitch` | 19 |
| | | `backfaceCulling` | 20 |
| | | `heliBladeDamage` | 21 |
| | | `disableCrouch` | 22 |

`driveBy`, `backfaceCulling` and `heliBladeDamage` are the server's
"disable" options: setting them to `true` disables drive-by shooting,
backface culling and helicopter blade damage.

## DisconnectReason

The `reason` of `onPlayerDisconnect`.

| Key | Value |
|---|---|
| `timeout` | 0 |
| `quit` | 1 |
| `kick` | 2 |
| `ban` | 2 |
| `kickBan` | 2 |
| `crash` | 3 |
| `ac` | 4 |

`ban` and `kickBan` equal `kick`: the server reports kicks and bans with the
same value. `ac` is an anti-cheat disconnect.

## BodyPart

The `bodyPart` of `onPlayerKill`.

| Key | Value |
|---|---|
| `body` | 0 |
| `torso` | 1 |
| `leftArm` | 2 |
| `rightArm` | 3 |
| `leftLeg` | 4 |
| `rightLeg` | 5 |
| `head` | 6 |
| `inVehicle` | 7 |

## PlayerState

`player.state` and the states passed to `onPlayerStateChange`.

| Key | Value |
|---|---|
| `none` | 0 |
| `normal` | 1 |
| `aim` | 2 |
| `driver` | 3 |
| `passenger` | 4 |
| `enterDriver` | 5 |
| `enterPassenger` | 6 |
| `exit` | 7 |
| `unspawned` | 8 |

## PlayerUpdate

The update type passed to `onPlayerUpdate`.

| Key | Value |
|---|---|
| `normal` | 0 |
| `aiming` | 1 |
| `driver` | 2 |
| `passenger` | 3 |

## PlayerOption

Used by `player:getOption` and `player:setOption`.

| Key | Value |
|---|---|
| `controllable` | 0 |
| `driveBy` | 1 |
| `whiteScanLines` | 2 |
| `greenScanLines` | 3 |
| `widescreen` | 4 |
| `showMarkers` | 5 |
| `canAttack` | 6 |
| `hasMarker` | 7 |
| `showOnRadar` | 7 |
| `chatTags` | 8 |
| `drunkEffects` | 9 |
| `bleeding` | 10 |

`showOnRadar` is another name for `hasMarker`.

## PlayerVehicle

`player.vehicleStatus`.

| Key | Value |
|---|---|
| `outside` | 0 |
| `entering` | 1 |
| `exiting` | 2 |
| `inside` | 3 |

## VehicleUpdate

The update type passed to `onVehicleUpdate`.

| Key | Value |
|---|---|
| `driverSync` | 0 |
| `otherSync` | 1 |
| `position` | 2 |
| `health` | 4 |
| `color` | 5 |
| `colour` | 5 |
| `rotation` | 6 |

## VehicleOption

Used by `vehicle:getOption` and `vehicle:setOption`.

| Key | Value |
|---|---|
| `lockDoors` | 0 |
| `alarm` | 1 |
| `lights` | 2 |
| `radioLocked` | 3 |
| `ghost` | 4 |
| `siren` | 5 |
| `singleUse` | 6 |
| `engine` | 7 |
| `boot` | 8 |
| `bonnet` | 9 |

`engine` is the server's "engine disabled" option: `true` turns the engine
off. `boot` and `bonnet` are open when `true`.

## VehicleSpeed

Which speed `vehicle:getSpeed` and `vehicle:setSpeed` use.

| Key | Value |
|---|---|
| `normal` | 1 |
| `normalRelative` | 2 |
| `turn` | 3 |
| `turnRelative` | 4 |

## PickupOption

Used by `pickup:getOption` and `pickup:setOption`.

| Key | Value |
|---|---|
| `singleUse` | 0 |
| `forceSize` | 2147483647 |

`forceSize` is a placeholder from the server SDK, not a real option.

## EntityType

The entity type passed to `onEntityPoolChange` and
`onEntityStreamingChange`.

| Key | Value |
|---|---|
| `vehicle` | 1 |
| `object` | 2 |
| `pickup` | 3 |
| `radio` | 4 |
| `player` | 5 |
| `blip` | 7 |
| `checkpoint` | 8 |

## NetworkStatistics

Used by `player:getNetworkStatistics(option)`. That function needs plugin
API 2.1; on an older server it raises
`GetNetworkStatistics is not supported by this server version`.

| Key | Value | Key | Value |
|---|---|---|---|
| `dataSentPerSecond` | 0 | `dataSentTotal` | 6 |
| `dataResentPerSecond` | 1 | `dataResentTotal` | 7 |
| `dataReceivedPerSecond` | 2 | `dataReceivedTotal` | 8 |
| `dataDiscardedPerSecond` | 3 | `dataDiscardedTotal` | 9 |
| `allBytesSentPerSecond` | 4 | `allBytesSentTotal` | 10 |
| `allBytesReceivedPerSecond` | 5 | `allBytesReceivedTotal` | 11 |
| `packetLossPerSecond` | 15 | `packetLossTotal` | 16 |
| `messagesWaiting` | 12 | `messagesResending` | 13 |
| `bytesResending` | 14 | | |

## Example

```lua
Event.bind("onPlayerKill", function(killer, player, reason, bodyPart)
    if bodyPart == BodyPart.head then
        Player.msgAll(killer.name .. " shot " .. player.name .. " in the head")
    end
end)

Event.bind("onPlayerDisconnect", function(player, reason)
    if reason == DisconnectReason.crash then
        print(player.name .. " crashed")
    end
end)
```

## Removed globals

These 2.x globals no longer exist. Each name is still defined, but any use
(reading a field, assigning a field or calling it) raises an error that names
the replacement, for example
`MySQL was removed in 3.0: use require "luasql.mysql" (see docs/MIGRATION-v3.md#mysql)`.

| Global | Replacement named in the error |
|---|---|
| `MySQL` | `require "luasql.mysql"` |
| `SQLite`, `SqLite`, `SQLiteDatabase` | `require "luasql.sqlite3"` |
| `Remote` | `require "http"` |
| `JSON` | `require "cjson"` |
| `Thread` | none: Lua runs on the server's thread only |
| `dbg` | none: it blocked the server waiting for console input |

See [MIGRATION-v3.md](../MIGRATION-v3.md#removed) for details.
