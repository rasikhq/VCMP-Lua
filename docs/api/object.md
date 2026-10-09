# Object

An `Object` handle names a map object the scripts or the server created.
Objects can move, rotate and fade, and can report when a player shoots or
touches them. For `id`, `valid`, `data`, `findByID`, `getActive`, `count`,
`type` and the argument rules, see [entities](entities.md).

## Creating

```
Object.create(model, world, x, y, z[, alpha])
Object.create(model, world, {x, y, z[, alpha]})
```

- `model` and `world` are 32-bit integers.
- `alpha` is 0 to 255 and defaults to 255. In the table form it is the
  table's fourth element; an argument after the table is ignored.
- Returns the new object's handle. If the server refuses, it raises
  `'create' failed: ...`.

`Object.new(...)` and `Object(...)` take the same arguments and do the same.

The object stays until `object:destroy()`, the server deletes it, or
`Server.reload()` runs. Dropping the handle deletes nothing.

## Static functions

Only the common ones: see [entities](entities.md).

## Methods

### object:destroy()

Deletes the object. Returns `true`, and the handle is dead afterwards. If
the server refuses, it returns `false` and the object stays.

### object:moveTo(x, y, z[, ms]) / object:moveBy(x, y, z[, ms])

Moves the object to a position, or by an offset, over `ms` milliseconds
(default 0). Also accepts `({x, y, z[, ms]})` and `({x, y, z}, ms)`. When
both are given, the argument after the table wins over the table's fourth
element. `ms` is 0 to 4294967295. Returns `true`, or `false` if the server
refuses.

### object:rotateTo(rotation[, ms]) / object:rotateBy(rotation[, ms])

Rotates the object to an angle, or by an angle, over `ms` milliseconds
(default 0). `rotation` must be a table:

- `{x, y, z}`: Euler angles.
- `{x, y, z, w}`: a quaternion. Any table with four or more elements is
  read as a quaternion.

Returns `true`, or `false` if the server refuses.

### object:setAlpha(alpha[, ms])

Fades the object to `alpha` over `ms` milliseconds (default 0). Returns
`true`, or `false` if the server refuses.

### object:getAlpha()

Returns the alpha. Same as `object.alpha`.

### object:getModel()

Returns the model. Same as `object.model`.

### object:streamedForPlayer(player)

Returns `true` if the object is streamed in for `player`.

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `model` | integer | read | |
| `world` | integer | read/write | |
| `alpha` | integer | read/write | Assigning sets it at once, with no fade. |
| `position` | `{x, y, z}` | read/write | |
| `angle` | `{x, y, z}` | read/write | Euler angles. Assigning rotates the object to this angle at once. |
| `trackShots` | boolean | read/write | Whether `onObjectShot(object, player, weapon)` fires for this object. |
| `trackTouch` | boolean | read/write | Whether `onObjectTouch(object, player)` fires for this object. |

## Example

```lua
local gate = Object.create(6400, 0, -1001.5, 196.2, 11.4)
gate.data.open = false
gate.trackShots = true

Event.bind("onObjectShot", function(object, player, weapon)
    if object ~= gate then return end
    local position = gate.position
    if gate.data.open then
        gate:moveTo(position[1], position[2], position[3] - 5, 2000)
    else
        gate:moveTo({ position[1], position[2], position[3] + 5 }, 2000)
    end
    gate.data.open = not gate.data.open
end)
```
