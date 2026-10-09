# Timers

`Timer` calls a function after a delay, once, a set number of times, or until
it is destroyed. Timers run on the server's main thread during each server
frame, so they never run at the same time as other script code.

## Functions and members

### Timer.create(fn, interval, repeats, ...)

Creates a timer that calls `fn(...)` every `interval` milliseconds.

- `fn`: the function to call.
- `interval`: milliseconds between calls, an integer from 0 to 2^40. An
  integral float such as `1000 / 2` is accepted. With 0 the timer runs once
  per server frame.
- `repeats`: how many calls in all, a positive integer, or -1 to repeat until
  the timer is destroyed. Required.
- `...`: arguments passed to `fn` on every call. They are stored when the
  timer is created and passed again unchanged each time.

Returns a `Timer` handle.

The first call is due `interval` milliseconds after `Timer.create`.

Errors:

- `Timer.create: argument 1 must be a function (got <type>)`
- `Timer.create: argument 2 (interval) must be an integer (got <value or type>)`
- `Timer.create: argument 3 (repeat count) must be an integer (got <value or type>)`
- `timer interval must be between 0 and 2^40 milliseconds`
- `timer repeat count must be -1 (forever) or positive`

### Timer.destroy(timer)

Stops the timer. It is never called again. Also callable as
`timer:destroy()`.

Returns `true`, or `false` if the timer had already ended or been destroyed.

A timer may destroy itself inside its own callback (`thisTimer:destroy()`).
The callback runs to its end, then the timer is removed.

### timer.active

`true` while the timer exists; `false` once it has made its last call or was
destroyed. Read-only.

During the last call of a timer with a repeat count, `active` is still
`true`.

### tostring(timer)

`Timer(<id>)`, where the id is a number that identifies the timer. Ids start
at 1 again after `Server.reload()`.

### thisTimer

A global that holds the running timer's handle while its callback runs. It is
restored to its previous value (usually `nil`) when the callback returns or
raises an error.

## When timers run

- Due timers run once per server frame, before `onServerFrame`. A timer can
  therefore not run more often than the server's frame rate.
- In one frame, the due timers run earliest-due first; timers due at the same
  time run in creation order.
- The plugin collects the due timers before it calls any of them. A timer
  created inside a timer callback waits for the next frame, even with an
  interval of 0. A timer destroyed by an earlier callback in the same frame
  does not run.
- Timers keep a fixed rate. If a timer falls behind, for example after a
  long frame, it runs once and then keeps its rate from that moment; missed
  calls are not made up.
- Times come from a 64-bit monotonic millisecond clock, so timers keep
  working however long the server runs.
- A callback that raises an error is logged as `Timer callback: <error>` with
  a traceback. The call still counts, and the timer keeps going.
- A timer created inside a coroutine keeps running after the coroutine ends.
- `Server.reload()` and server shutdown drop every timer without calling it.
- While the plugin shuts down or reloads, `Timer.create`, `Timer.destroy` and
  `timer.active` raise `runtime shutting down`.

## Example

```lua
-- Counts down in a player's chat, then stops on its own.
local function countdown(player, seconds)
    local left = seconds
    Timer.create(function(p)
        if not p.valid then
            thisTimer:destroy() -- the player left
            return
        end
        left = left - 1
        p:msg(left > 0 and ("Starting in " .. left) or "Go!")
    end, 1000, seconds, player)
end

-- Saves every five minutes until destroyed.
local autosave = Timer.create(function()
    print("autosave")
end, 5 * 60 * 1000, -1)

Event.bind("onServerShutdown", function()
    autosave:destroy()
end)
```
