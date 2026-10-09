# Migrating from v1 to v2

Draft: each phase adds the changes it makes; phase 5 completes this guide.

## Events

- `Event.bind`, `Event.unbind`, `Event.create`, `Event.cancel` and
  `Event.trigger` keep their names and arguments.
- Mistakes raise errors instead of logging and returning false: an unknown
  event name, a handler that is not a function, `Event.cancel()` outside an
  event handler. Binding a function twice and unbinding a function that is
  not bound still return false.
- `Event.cancel()` applies to the innermost event being dispatched. In v1
  one global flag was shared by nested events.
- `Event.trigger` returns false when a handler cancelled the event (v1
  always returned true).
- `onServerInit` also runs after `Server.reload()`.
- At server shutdown, `onPlayerDisconnect` runs for every player still
  online, with reason 0, after `onServerShutdown`.

## Timers

- `Timer.create(fn, interval, repeats, ...)` and `Timer.destroy(timer)` keep
  their names; `timer:destroy()` and `timer.active` are new.
- The 50 ms minimum interval is gone (0 runs the timer every frame).
  Invalid arguments raise errors instead of returning nil.
- Timers no longer stop after about 71.6 minutes of uptime.

## Entities

- A script holds a handle; the same entity is always the same handle, so
  `==` and table keys work. Using a handle after its entity is gone raises
  "<kind> no longer exists" instead of crashing or touching another entity.

## New

- `Server.reload()` reloads `luaconfig.lua` and every script.
