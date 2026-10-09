# Pickup

A `Pickup` handle names a pickup: a weapon, health, armour or other item a
player can collect. For `id`, `valid`, `data`, `findByID`, `getActive`,
`count`, `type` and the argument rules, see [entities](entities.md).

## Creating

```
Pickup.create(model, world, quantity, x, y, z[, alpha[, automatic]])
Pickup.create(model, world, quantity, {x, y, z}[, alpha[, automatic]])
```

- `model`, `world` and `quantity` are 32-bit integers.
- `alpha` is 0 to 255 and defaults to 255. In the table form it is the
  argument after the table, not an element of it.
- `automatic` is a boolean and defaults to `true`.
- Returns the new pickup's handle. If the server refuses, it raises
  `'create' failed: ...`.

`Pickup.new(...)` and `Pickup(...)` take the same arguments and do the same.

The pickup stays until `pickup:destroy()`, the server deletes it, or
`Server.reload()` runs. Dropping the handle deletes nothing.

## Static functions

Only the common ones: see [entities](entities.md).

## Methods

### pickup:destroy()

Deletes the pickup. Returns `true`, and the handle is dead afterwards. If
the server refuses, it returns `false` and the pickup stays.

### pickup:respawn()

Respawns the pickup. Returns `true`, or `false` if the server refuses.

### pickup:getOption(option)

Returns whether the option is on. `option` is a `PickupOption` value, for
example `PickupOption.singleUse`.

### pickup:setOption(option, on)

Turns the option on or off. `on` must be a boolean. Returns `true`, or
`false` if the server refuses.

### pickup:getModel()

Returns the model. Same as `pickup.model`.

### pickup:streamedForPlayer(player)

Returns `true` if the pickup is streamed in for `player`.

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `model` | integer | read | |
| `quantity` | integer | read | |
| `world` | integer | read/write | |
| `alpha` | integer | read/write | |
| `auto` | boolean | read/write | Whether the pickup is automatic. |
| `autoTimer` | integer | read/write | Milliseconds, 0 to 4294967295. |
| `position` | `{x, y, z}` | read/write | |

Related events: `onPickupPickAttempt(pickup, player)`,
`onPickupPicked(pickup, player)` and `onPickupRespawn(pickup)`.

## Example

```lua
local bonus = Pickup.create(366, 0, 1, { -657.1, 762.3, 11.6 })
bonus.autoTimer = 30000
bonus.data.taken = 0

Event.bind("onPickupPicked", function(pickup, player)
    if pickup == bonus then
        pickup.data.taken = pickup.data.taken + 1
        player:msg("Bonus taken " .. pickup.data.taken .. " times.")
    end
end)
```
