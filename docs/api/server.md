# Server, Map, Radio, Weapon, Blip and Sound

These globals control the server as a whole: its settings, the game world,
radio streams, weapon data, radar blips and sounds. Positions, colours,
integers and booleans follow the conventions in [entities.md](entities.md).

Unless a row says otherwise, a function that returns a boolean returns `true`
on success and `false` when the server refused the request. Any other error
the server reports raises an error such as
`'createStream' failed: argument out of bounds`. A function whose server
call is missing from the running server version raises
`<ServerFunction> is not supported by this server version`.

## Server

`Server` is a single global object. Call its functions with a dot
(`Server.getName()`), not a colon. Each property can also be read and written
with the matching function where one exists (`Server.name`,
`Server.getName()`, `Server.setName(text)`).

### Lifecycle

| Function | Returns | Notes |
|---|---|---|
| `Server.type()` | string | Always `"Server"`. |
| `Server.reload()` | nothing | Reloads the config and every script at the end of the server frame. See below. |
| `Server.shutdown()` | nothing | Asks the server to shut down. See below. |

#### Server.reload()

`Server.reload()` only sets a flag. At the end of the current server frame,
once no Lua code is running, the plugin:

1. Reads `luaconfig.lua` again and sets up a new Lua state. If this fails
   (for example the config is now invalid), the error is logged and the old
   scripts keep running unchanged.
2. Deletes the vehicles, objects, pickups, checkpoints and blips the old
   scripts created, and removes the key binds they registered. Key binds of
   other plugins are not touched.
3. Drops all timers and event handlers. Pending HTTP requests are dropped
   without calling their callbacks. The old Lua state is closed, so all Lua
   values (including entity `data` tables) are lost.
4. Runs the scripts in the new state, then fires `onServerInit`.

Players stay connected. The new scripts see them through the normal handles
(`Player.getActive()` and so on), but `onPlayerConnect` does not fire again.
`onServerShutdown` does not fire on a reload.

Server state that is not an entity stays as it was: player classes (the
server API cannot remove them), server settings and options, hidden map
objects, weapon data and radio streams.

If a shutdown is pending, the reload does not run.

#### Server.shutdown()

Asks the server to shut down. When the server does so, the plugin fires
`onServerShutdown`, then `onPlayerDisconnect(player, DisconnectReason.timeout)`
for every player still connected, then closes the Lua state.

### Settings and options

| Function | Returns | Notes |
|---|---|---|
| `Server.getOption(option)` | boolean | `option` is a [`ServerOption`](enums.md#serveroption) value. Raises `'getOption' failed: argument out of bounds` for an unknown option. |
| `Server.setOption(option, on)` | boolean | `on` must be `true` or `false`. |
| `Server.getSettings()` | table | `{maxPlayers, port, serverName, serverPassword, flags}`. All but `serverName` and `serverPassword` are integers. |
| `Server.getName()` / `Server.setName(text)` | string / nothing | Same as `Server.name`. |
| `Server.getMaxPlayers()` / `Server.setMaxPlayers(count)` | integer / nothing | Same as `Server.maxPlayers`. |
| `Server.getGame()` / `Server.setGame(text)` | string / nothing | Same as `Server.gamemode`. |
| `Server.getPassword()` / `Server.setPassword(text)` | string / nothing | Same as `Server.password`. |

### Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `name` | string | read/write | Server name. |
| `maxPlayers` | integer | read/write | 0 to 4294967295. |
| `gamemode` | string | read/write | Game mode text. |
| `password` | string | read/write | Server password. |
| `hour` | integer | read/write | Game clock hour. |
| `minute` | integer | read/write | Game clock minute. |
| `timeRate` | integer | read/write | |
| `weather` | integer | read/write | Weather id. |
| `gravity` | number | read/write | |
| `waterLevel` | number | read/write | |
| `gameSpeed` | number | read/write | |
| `flightAltitude` | number | read/write | Maximum flight altitude. |
| `vehicleRespawnHeight` | number | read/write | Height at which vehicles are forced to respawn. |
| `killDelay` | integer | read/write | Delay of the kill command. |
| `fallTimer` | integer | read/write | 0 to 65535. Needs a server that has `SetFallTimer`/`GetFallTimer` (added in 04rel005); older servers raise `GetFallTimer is not supported by this server version` (or `SetFallTimer ...`). |
| `wastedSettings` | table | read/write | See below. |

A value of the wrong type or range raises, for example
`bad value for 'maxPlayers' (value -1 out of range [0, 4294967295])`.

`Server.wastedSettings` reads as a new table with the fields `deathTimer`,
`fadeTimer`, `fadeInSpeed`, `fadeOutSpeed`, `fadeColour`, `corpseFadeStart` and
`corpseFadeTime`. `fadeInSpeed` and `fadeOutSpeed` are numbers, the others are
integers from 0 to 4294967295 (`fadeColour` is a colour). Changing the returned table changes
nothing; assign a table back. An assigned table may hold only some fields;
the others keep their current values:

```lua
Server.wastedSettings = { deathTimer = 5000 }
```

Assigning something other than a table raises an error.

### Classes and spawn screen

| Function | Returns | Notes |
|---|---|---|
| `Server.addClass(...)` | integer | Adds a player class. See below. |
| `Server.setClassPosition(position)` | nothing | Where the player stands on the class selection screen. |
| `Server.setClassCameraPosition(position)` | nothing | Camera position on the class selection screen. |
| `Server.setClassCameraLook(position)` | nothing | Point the camera looks at. |

#### Server.addClass

```
Server.addClass(team, colour, skin, x, y, z, angle [, weapon1, ammo1 [, weapon2, ammo2 [, weapon3, ammo3]]])
Server.addClass(team, colour, skin, {x, y, z}, angle [, weapons...])
Server.addClass(team, colour, skin, {x, y, z, angle} [, weapons...])
```

Returns the new class id. Up to three weapon and ammo pairs may follow the
position and angle; missing ones are 0. In the third form the angle is the
fourth element of the table, so no separate angle argument follows. If the
server refuses, it raises `'addClass' failed: <reason>` (for example
`pool exhausted`) or `'addClass' failed: the server refused`.

Classes cannot be removed, and they survive `Server.reload()`.

### Bans

| Function | Returns | Notes |
|---|---|---|
| `Server.banIP(ip)` | boolean | `false` if the IP was banned already. |
| `Server.unbanIP(ip)` | boolean | `false` if the IP was not banned. |
| `Server.isIPBanned(ip)` | boolean | |

`ip` is a string such as `"1.2.3.4"`.

### World

| Function | Returns | Notes |
|---|---|---|
| `Server.createExplosion(...)` | boolean | See below. |
| `Server.getSkinID(name)` | integer | Skin id for a name such as `"cop"` or `"Army"`, or -1. The name is matched loosely by its letters, ignoring case. |
| `Server.getSkinName(id)` | string or nil | `nil` for an unknown id. |

#### Server.createExplosion

```
Server.createExplosion(world, type, position [, creator [, atGroundLevel]])
```

`position` is a table `{x, y, z}` or three numbers. `creator` is a `Player`
handle or `nil` (no creator); a player who has left raises
`player no longer exists`. `atGroundLevel` is a boolean and defaults to
`false`.

## Map

| Function | Returns | Notes |
|---|---|---|
| `Map.setBounds(maxX, minX, maxY, minY)` | nothing | World boundaries. Note the argument order. |
| `Map.getBounds()` | table | `{max_x, min_x, max_y, min_y}`. |
| `Map.hideObject(model, position)` | nothing | Hides a map object. `position` is in world units and is rounded to the nearest tenth. |
| `Map.hideObjectRaw(model, x, y, z)` | nothing | Same, with the position already in tenths of a unit (integers from -32768 to 32767). |
| `Map.showMapObject(model, position)` | nothing | Shows an object hidden with `hideObject`. |
| `Map.showObject(model, position)` | nothing | Same as `showMapObject`. |
| `Map.showAllObjects()` | nothing | Shows every hidden map object. |
| `Map.getDistrictName(x, y)` | string | Also takes a table `{x, y[, z]}`. Returns `"Vice City"` outside every named district. |

`hideObject`, `showMapObject` and `showObject` raise
`coordinate out of range` when a coordinate does not fit in tenths
(beyond about ±3276.7 units).

District names: Downtown Vice City, Vice Point, Washington Beach, Ocean
Beach, Escobar International Airport, Starfish Island, Prawn Island, Leaf
Links, Junkyard, Viceport, Little Havana, Little Haiti.

## Radio

| Function | Returns | Notes |
|---|---|---|
| `Radio.createStream([id,] name, url [, listed])` | boolean | See below. |
| `Radio.destroyStream(id)` | boolean | Removes a radio stream. |

### Radio.createStream

```
Radio.createStream(name, url [, listed])
Radio.createStream(id, name, url [, listed])
```

If the first argument is a number, it is the stream id; otherwise the server
picks the id. `listed` must be a boolean and defaults to `true`. Returns
`true` on success. A bad id raises
`'createStream' failed: argument out of bounds`.

## Weapon

| Function | Returns | Notes |
|---|---|---|
| `Weapon.data(weapon, field)` | number | Reads a weapon data field. |
| `Weapon.data(weapon, field, value)` | nothing | Sets a weapon data field. |
| `Weapon.isFieldModified(weapon, field)` | boolean | `true` if the field differs from the default. |
| `Weapon.resetField(weapon, field)` | boolean | Restores one field. |
| `Weapon.reset(weapon)` | boolean | Restores every field of one weapon. |
| `Weapon.resetAll()` | nothing | Restores every weapon. |
| `Weapon.getName(id)` | string | Weapon or death reason name, such as `"M4"`; `"Unknown"` for an unknown id. |
| `Weapon.getID(name)` | integer | Weapon id for a name such as `"m4"` or `"Uzi"`, matched loosely by its first letters, ignoring case. 255 if unknown, 0 for an empty name. |

An unknown weapon or field raises, for example
`'data' failed: argument out of bounds`.

## Blip

`Blip` is an entity class like `Vehicle` or `Pickup`: `Blip.create` returns a
`Blip` handle. Blips created by the scripts are destroyed on
`Server.reload()`.

| Member | Returns | Notes |
|---|---|---|
| `Blip.create(...)` | Blip | See below. |
| `Blip.destroy(blip)` / `blip:destroy()` | boolean | `blip` is a handle or a blip id. `false` if there is no blip with that id. |
| `Blip.getInfo(blip)` / `blip:getInfo()` | table or nil | `{world, position, scale, color, sprite}`; `position` is `{x, y, z}`. `nil` if the server has no such blip. Takes a handle or an id. |
| `blip.id` / `blip:getID()` | integer | The blip id. |
| `blip.valid` | boolean | `false` once the blip is destroyed. |
| `blip.data` | table | A table for script data, dropped with the blip. |
| `Blip.findByID(id)` | Blip or nil | |
| `Blip.count()` | integer | Number of blips. |
| `Blip.getActive()` | table | `{[id] = blip}` of every blip. |
| `Blip.type()` / `blip:getType()` | string | `"Blip"`. |

Using a handle whose blip was destroyed raises `blip no longer exists`, except
for `valid` and `tostring`. A blip id passed as a number is not checked this
way: `Blip.destroy(id)` returns `false` and `Blip.getInfo(id)` returns `nil`
when there is no such blip. These common members are described in
[entities.md](entities.md).

### Blip.create

```
Blip.create(world, position, scale, colour, sprite)
Blip.create(index, world, position, scale, colour, sprite)
```

`position` is a table `{x, y, z}` or three numbers. `index` picks the blip
id; leave it out or pass -1 to let the server choose. With three numbers for
the position, the form with an index has eight arguments and the form
without has seven. If the server refuses, it raises
`'create' failed: <reason>` (for example `pool exhausted`) or
`'create' failed: the server refused`.

## Sound

| Function | Returns | Notes |
|---|---|---|
| `Sound.play(...)` | boolean | See below. |

### Sound.play

```
Sound.play(sound)
Sound.play(sound, position)
Sound.play(world, sound)
Sound.play(world, sound, position)
```

`position` is a table `{x, y, z}` or three numbers. Without a position the
sound is not positional. Without a world, the world is 0. With two numbers,
the first is the world: `Sound.play(2, 50)` plays sound 50 in world 2.

## Example

```lua
Event.bind("onServerInit", function()
    Server.name = "Vice City Freeroam"
    Server.hour = 12
    Server.minute = 0
    Server.setOption(ServerOption.friendlyFire, false)
    Server.addClass(1, 0xFF0000FF, 1, { -378.8, -537.4, 17.3, 90.0 }, 26, 500)
    Radio.createStream("Wave 103", "http://example.com/wave.mp3")
    Blip.create(0, { -378.8, -537.4, 17.3 }, 1, 0xFFFF00FF, 0)
end)

Event.bind("onPlayerCommand", function(player, command)
    if command == "reload" and player.admin then
        Player.msgAll("Reloading scripts...")
        Server.reload()
    elseif command == "boom" then
        Server.createExplosion(player.world, 0, player.position, player)
    end
end)
```
