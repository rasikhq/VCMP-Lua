# Events

Scripts react to the server through events. The `Event` table binds handler
functions to an event by name, and lets scripts create and trigger their own
events. Differences from 2.x are listed in the [migration guide](../MIGRATION-v3.md#events).

## Functions

Errors are raised as Lua errors with the script position, for example
`main.lua:12: unknown event 'onPlayerJoin'`.

### Event.bind(name, fn)

Adds `fn` to the handlers of the event `name`. Handlers run in the order they
were bound.

- `name`: a built-in event name (see [Server events](#server-events)) or the
  name of a custom event. Names are case-sensitive.
- `fn`: a function. Functions are compared by identity, so a new closure is a
  new handler.

Returns `true`, or `false` if `fn` is already bound to this event.

Errors: `unknown event '<name>'`; `Event.bind: argument 2 must be a function
(got <type>)`.

### Event.unbind(name, fn)

Removes `fn` from the handlers of the event `name`. Pass the same function
value that was bound.

Returns `true`, or `false` if `fn` is not bound to this event.

Errors: `unknown event '<name>'`; `Event.unbind: argument 2 must be a function
(got <type>)`.

### Event.create(name)

Adds a custom event called `name`. Any string works as a name; the `on` prefix
is a convention, not a rule.

Returns `true`, or `false` if an event with that name exists already. This
includes the built-in names: `Event.create("onPlayerConnect")` returns
`false`.

Custom events last until the scripts are reloaded. They cannot be removed.

### Event.trigger(name, ...)

Calls the handlers of the event `name` with the arguments `...`, in bind order.
It returns once every handler has run.

Returns `false` if a handler called `Event.cancel()`, else `true` (also when
the event has no handlers).

Errors: `unknown event '<name>'`.

`Event.trigger` also accepts a built-in event name. That runs the Lua handlers
only; the server is not involved, and cancelling has no effect on the server.

### Event.cancel()

Cancels the innermost event being dispatched:

- The handlers after the current one do not run. The current handler
  continues to its end.
- `Event.trigger` returns `false` for that dispatch.
- For a [cancellable server event](#cancellable-events), the plugin tells the
  server to refuse the action.

Each dispatch has its own cancel flag. If a handler triggers another event
(or causes a server event, such as a kick that dispatches
`onPlayerDisconnect`), `Event.cancel()` inside that inner dispatch cancels
only the inner one. The outer event goes on.

Returns nothing.

Errors: `Event.cancel() called outside an event handler`.

## Dispatch rules

- A dispatch calls the handlers that were bound when it started. A handler
  bound during the dispatch runs from the next dispatch on.
- A handler unbound during a dispatch, before its turn, does not run. A
  handler may unbind itself.
- A handler that raises an error is logged with its traceback, as
  `<event name>: <error>`, and the next handler runs.
- Return values of handlers are ignored.
- Handlers run on the thread that dispatches the event. For server events
  that is the main thread; `Event.trigger` called inside a coroutine runs the
  handlers in that coroutine.
- A handler bound inside a coroutine stays bound after the coroutine ends.
- Server events can nest. For example, `player:kick()` in a handler makes the
  server report the disconnect at once, so the `onPlayerDisconnect` handlers
  run before `kick()` returns, and the player's handle is no longer valid
  after it.
- While the plugin shuts down or reloads, every `Event` function raises
  `runtime shutting down` (this can happen in a `__gc` metamethod).

## Server events

Entity arguments are handles (`Player`, `Vehicle`, `Object`, `Pickup`,
`Checkpoint`, `Bind`). An entity the scripts have not seen yet, for example
one another plugin created, is passed as a handle too. An id of -1, or one
outside the entity pool, is passed as `nil`.

The integer arguments that name a kind of value match the constant tables:
`DisconnectReason`, `BodyPart`, `PlayerState`, `PlayerUpdate`,
`VehicleUpdate` and `EntityType`.

### Server

| Event | Arguments | Notes |
|---|---|---|
| `onServerInit` | none | After the scripts in `luaconfig.lua` have run, at server start and after every `Server.reload()`. |
| `onServerFrame` | `elapsed` | Every server frame. `elapsed` is the frame time in seconds. Runs after the due timers, finished HTTP requests and one Copas step. |
| `onServerShutdown` | none | When the server shuts down. Followed by `onPlayerDisconnect` for every player still online. Not called by `Server.reload()`. |
| `onPluginCommand` | `command, text` | A plugin sent a plugin command. `command` is its integer identifier. Cancellable. |
| `onServerPerformanceReport` | `count, descriptions, times` | `descriptions` (strings) and `times` (integers) are arrays of `count` entries. |
| `onEntityPoolChange` | `entityType, id, deleted` | An entity was created (`deleted` is `false`) or deleted (`true`), by any plugin. `entityType` is an `EntityType` value and `id` an integer, not a handle. Not called while the scripts reload. |
| `onEntityStreamingChange` | `player, entityType, entityId, deleted` | An entity streamed in for `player` (`deleted` is `false`) or out (`true`). `entityType` is an `EntityType` value and `entityId` an integer. Only on servers whose plugin API has this callback. |

### Players

| Event | Arguments | Notes |
|---|---|---|
| `onPlayerConnection` | `name, nameSize, password, ip` | Before a player joins; there is no handle yet. All strings, except `nameSize`, the size of the server's name buffer. Cancellable: refuses the connection. |
| `onPlayerConnect` | `player` | A player has joined. |
| `onPlayerDisconnect` | `player, reason` | `reason` is a `DisconnectReason` value. The handle stays valid during the event and becomes invalid after it. |
| `onPlayerModuleList` | `player, list` | The player's module list, as one string. |
| `onPlayerRequestClass` | `player, offset` | `offset` as the server reports it. Cancellable. |
| `onPlayerRequestSpawn` | `player` | Cancellable: the player does not spawn. |
| `onPlayerSpawn` | `player` | |
| `onPlayerKill` | `killer, player, reason, bodyPart` | `player` was killed by `killer`, a connected player. `reason` is the weapon or death reason id; `bodyPart` a `BodyPart` value. |
| `onPlayerWasted` | `player, reason` | `player` died with no connected killer. See [Death reasons](#death-reasons). |
| `onPlayerUpdate` | `player, update` | `update` is a `PlayerUpdate` value. Many times per second. |
| `onPlayerRequestEnterVehicle` | `player, vehicle, slot` | `slot` is the seat. Cancellable. |
| `onPlayerEnterVehicle` | `player, vehicle, slot` | |
| `onPlayerExitVehicle` | `player, vehicle` | |
| `onPlayerNameChange` | `player, oldName, newName` | |
| `onPlayerStateChange` | `player, oldState, newState` | `PlayerState` values. |
| `onPlayerActionChange` | `player, oldAction, newAction` | Integer action ids. |
| `onPlayerFireChange` | `player, onFire` | `onFire` is a boolean. |
| `onPlayerCrouchChange` | `player, crouching` | `crouching` is a boolean. |
| `onPlayerGameKeysChange` | `player, oldKeys, newKeys` | Key bit masks, as unsigned 32-bit integers. |
| `onPlayerBeginTyping` | `player` | |
| `onPlayerFinishTyping` | `player` | |
| `onPlayerAwayChange` | `player, away` | `away` is a boolean. |
| `onPlayerMessage` | `player, text` | A chat message. Cancellable. |
| `onPlayerCommand` | `player, command, args, text` | See [onPlayerCommand](#onplayercommand). Cancellable. |
| `onPlayerPM` | `player, target, text` | A private message to `target`. Cancellable. |
| `onPlayerKeyDown` | `player, bind` | A key bind (`Bind`) was pressed. |
| `onPlayerKeyUp` | `player, bind` | A key bind was released. |
| `onPlayerSpectate` | `player, target` | `target` is `nil` for an id of -1. |
| `onPlayerCrashReport` | `player, report` | The crash report text. |
| `onClientData` | `player, stream, size` | Data from a client script. `stream` is a `Stream` holding the `size` bytes. Every handler gets the same `Stream`, so a handler that reads it moves the read position for the handlers after it. |

### Vehicles, objects, pickups, checkpoints

| Event | Arguments | Notes |
|---|---|---|
| `onVehicleUpdate` | `vehicle, update` | `update` is a `VehicleUpdate` value. Many times per second. |
| `onVehicleExplode` | `vehicle` | |
| `onVehicleRespawn` | `vehicle` | |
| `onObjectShot` | `object, player, weapon` | `weapon` is the weapon id. |
| `onObjectTouch` | `object, player` | |
| `onPickupPickAttempt` | `pickup, player` | Cancellable: the player cannot pick it up. |
| `onPickupPicked` | `pickup, player` | |
| `onPickupRespawn` | `pickup` | |
| `onCheckpointEnter` | `checkpoint, player` | |
| `onCheckpointExit` | `checkpoint, player` | |

### Cancellable events

Calling `Event.cancel()` in a handler of these events makes the plugin answer
the server with a refusal:

`onPlayerConnection`, `onPlayerRequestClass`, `onPlayerRequestSpawn`,
`onPlayerRequestEnterVehicle`, `onPlayerMessage`, `onPlayerCommand`,
`onPlayerPM`, `onPickupPickAttempt`, `onPluginCommand`.

For every other event, `Event.cancel()` only stops the remaining handlers.

### onPlayerCommand

The command line (without the `/`) is split on spaces.

- `command`: the first word, or `nil` if the text is empty or only spaces.
- `args`: an array of the remaining words, or `nil` if there are none.
- `text`: the rest of the line from the first argument on, with its spaces
  kept; `""` if there are no arguments.

`/give 5 100` gives `"give"`, `{"5", "100"}`, `"5 100"`. `/help` gives
`"help"`, `nil`, `""`.

### Death reasons

`onPlayerWasted` passes some reasons in a simplified form:

| Server reason | Passed as |
|---|---|
| 43 or 50 | 43 (drowned) |
| 39 with body part `BodyPart.inVehicle` | 39 (car crash) |
| 39 (other body parts), 40 or 44 | 44 (fell) |
| anything else | unchanged |

`onPlayerKill` passes the reason unchanged.

### Start, reload and shutdown

- Players and entities that exist when the scripts load (for example after
  `Server.reload()`) are adopted without `onPlayerConnect` or
  `onEntityPoolChange`. Use `Player.getActive()` in `onServerInit` to find
  them.
- At server shutdown the order is: `onServerShutdown`, then
  `onPlayerDisconnect` with reason `DisconnectReason.timeout` (0) for every
  player still online. Scripts can save player data in their disconnect
  handler in every case.
- `Server.reload()` does not call `onServerShutdown` or `onPlayerDisconnect`.
  The new scripts get `onServerInit`.
- If the server version has no callback for an event, the plugin logs
  `this server version has no <callback> callback` at load, and that event
  never fires.

## Example

```lua
local muted = {}

Event.create("onPlayerMuted")

Event.bind("onPlayerMuted", function(player, by)
    print(player.name .. " was muted by " .. by.name)
end)

Event.bind("onPlayerCommand", function(player, command, args, text)
    if command == "mute" and args then
        local id = tonumber(args[1], 10)
        local target = id and Player.findByID(id)
        if target then
            muted[target.id] = true
            Event.trigger("onPlayerMuted", target, player)
        end
        Event.cancel()
    end
end)

Event.bind("onPlayerMessage", function(player, text)
    if muted[player.id] then
        player:msg("You are muted.", 0xFF0000FF)
        Event.cancel() -- the server does not send the message
    end
end)

Event.bind("onPlayerDisconnect", function(player, reason)
    muted[player.id] = nil
end)
```
