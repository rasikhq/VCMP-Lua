# API reference

Scripts run on the server's thread, inside protected calls: an error in a
script is logged with its traceback and never stops the server.

- [Configuration](../configuration.md): `luaconfig.lua`
- [Events](events.md): `Event` and every server event
- [Timers](timers.md): `Timer`
- [Entities](entities.md): handles, lifetime, and the conventions every
  binding follows (positions, colours, errors)
- [Player](player.md)
- [Vehicle](vehicle.md)
- [Object](object.md)
- [Pickup](pickup.md)
- [Checkpoint](checkpoint.md)
- [Bind](bind.md): key binds
- [Stream](stream.md): data to and from client scripts
- [Server, Map, Radio, Weapon, Blip, Sound](server.md)
- [Enums](enums.md)
- [Logger](logger.md)
- [Modules](modules.md): `http`, `Hash`, `sql`, LuaSQL, cjson, LuaSocket,
  Copas, lfs, inspect

Upgrading from v1: see the [migration guide](../MIGRATION-v2.md).
