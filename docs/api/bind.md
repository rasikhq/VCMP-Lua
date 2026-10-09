# Bind

A `Bind` handle names a key bind: one to three keys the server watches on
every client. When a player presses them, `onPlayerKeyDown(player, bind)`
fires. For `id`, `valid`, `data`, `findByID`, `getActive`, `count`, `type`
and the argument rules, see [entities](entities.md).

Key binds are shared by every plugin on the server, and the server reports
no event when one is removed. So every use of a bind handle first asks the
server whether the bind still exists. If not, the handle dies and the call
raises `bind no longer exists`. Until a handle is used, `bind.valid` can
still be `true` for a bind another plugin removed.

## Creating

```
Bind.create(signalOnRelease, key1[, key2[, key3]])
```

- `signalOnRelease` must be a boolean: whether the server also reports the
  release of the keys, with `onPlayerKeyUp(player, bind)`.
- The keys are 32-bit integers: the client's key codes, for example `0x42`
  for B. A missing key is 0.
- Returns the new bind's handle. It raises `'create' failed: no free key
  bind slot` when every slot is in use, and `'create' failed: request
  denied` when the server refuses.

`Bind.new(...)` and `Bind(...)` take the same arguments and do the same.

The bind stays until `bind:destroy()`, `Bind.clearAllBinds()` or
`Server.reload()`. Dropping the handle removes nothing.

## Static functions

### Bind.findByTag(tag)

Returns the first bind, in id order, whose `tag` equals `tag`, or `nil`.

### Bind.clearAllBinds()

Removes every bind the scripts created. Binds that other plugins
registered stay. Returns nothing.

The common static functions are in [entities](entities.md).
`Bind.getActive()` and `Bind.count()` also include binds other plugins
registered, once this plugin has seen them.

## Methods

### bind:destroy()

Removes the bind and returns `true`. The handle is dead afterwards. Returns
`false` if the server refuses; then the bind and its handle stay.

### bind:getData()

Returns a table:

| Field | Type | Notes |
|---|---|---|
| `keyOne` | integer | |
| `keyTwo` | integer | 0 if not used. |
| `keyThree` | integer | 0 if not used. |
| `signalsOnRelease` | boolean | |

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `tag` | string | read/write | A name of your choice for `Bind.findByTag`. Empty by default. Dropped with the bind. |

## Example

```lua
local menu = Bind.create(false, 0x4D) -- M
menu.tag = "menu"

Event.bind("onPlayerKeyDown", function(player, bind)
    if bind.tag == "menu" then
        player.data.menuOpen = not player.data.menuOpen
        player:msg(player.data.menuOpen and "Menu opened." or "Menu closed.")
    end
end)
```
