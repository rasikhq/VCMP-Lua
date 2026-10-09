# Checkpoint

A `Checkpoint` handle names a checkpoint marker, shown to one player or to
every player. The server reports when a player enters or leaves it. For
`id`, `valid`, `data`, `findByID`, `getActive`, `count`, `type` and the
argument rules, see [entities](entities.md).

## Creating

```
Checkpoint.create(player, world, isSphere, x, y, z, {r, g, b[, a]}, radius)
Checkpoint.create(player, world, isSphere, {x, y, z}, {r, g, b[, a]}, radius)
```

- `player` is the `Player` who sees it, or `nil` for every player.
- `world` is a 32-bit integer.
- `isSphere` must be a boolean.
- The colour is a table. Each channel is an integer from 0 to 255; `a`
  defaults to 255.
- `radius` is a number.
- Returns the new checkpoint's handle. If the server refuses, it raises
  `'create' failed: ...`.

`Checkpoint.new(...)` and `Checkpoint(...)` take the same arguments and do
the same.

The checkpoint stays until `checkpoint:destroy()`, the server deletes it,
or `Server.reload()` runs. Dropping the handle deletes nothing.

## Static functions

Only the common ones: see [entities](entities.md).

## Methods

### checkpoint:destroy()

Deletes the checkpoint. Returns `true`, and the handle is dead afterwards. If
the server refuses, it returns `false` and the checkpoint stays.

### checkpoint:getOwner()

Returns the player it was created for, or `nil` if every player sees it.
Same as `checkpoint.owner`.

### checkpoint:isSphere()

Returns `true` for a sphere. Same as `checkpoint.sphere`.

### checkpoint:streamedForPlayer(player)

Returns `true` if the checkpoint is streamed in for `player`.

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `owner` | Player or `nil` | read | `nil`: every player sees it. |
| `sphere` | boolean | read | |
| `world` | integer | read/write | |
| `radius` | number | read/write | |
| `color` | `{r, g, b, a}` | read/write | Channels 0 to 255. Assigning `{r, g, b}` keeps the current alpha. |
| `alpha` | integer | read/write | The colour's alpha. Assigning keeps `r`, `g` and `b`. |
| `position` | `{x, y, z}` | read/write | |

Related events: `onCheckpointEnter(checkpoint, player)` and
`onCheckpointExit(checkpoint, player)`.

## Example

```lua
Event.bind("onPlayerSpawn", function(player)
    if player.data.finish == nil then
        player.data.finish = Checkpoint.create(player, player.world, true,
            { 300.0, -200.0, 10.0 }, { 0, 255, 0 }, 4.0)
    end
end)

Event.bind("onCheckpointEnter", function(checkpoint, player)
    if checkpoint == player.data.finish then
        player:msg("Finished!")
        checkpoint:destroy()
        player.data.finish = nil
    end
end)
```
