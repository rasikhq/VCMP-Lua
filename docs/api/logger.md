# Logger

`Logger` writes messages to the plugin log at a chosen severity. The plugin
log goes to the server console and, if `log.file` is set in
[luaconfig.lua](../configuration.md#log), to a file.

## Functions

### Logger.debug(message)
### Logger.info(message)
### Logger.warn(message)
### Logger.error(message)
### Logger.critical(message)

Writes `message` at that level, unless the level is below the current minimum
level (see `Logger.setLevel`).

- `message`: a string or a number. The text is written as it is; it is not a
  format string.

Returns nothing.

Errors: `bad argument #1 to '<name>' (string expected, got <type>)` for any
other type.

There is no `Logger.trace`: scripts cannot write at the `trace` level, so
setting the level to `trace` shows the same messages as `debug`.

### Logger.setLevel(level)

Sets the minimum level: messages below it are not written.

- `level`: one of the names `"trace"`, `"debug"`, `"info"`, `"warn"`,
  `"error"`, `"critical"`, `"off"`, or a number from 0 to 5: 0 `debug`,
  1 `info`, 2 `warn`, 3 `error`, 4 `critical`, 5 `off`.

Returns nothing.

Errors:

- `bad argument #1 to 'setLevel' (level must be trace, debug, info, warn, error, critical, off or 0-5)`
  for an unknown name.
- `bad argument #1 to 'setLevel' (value <n> out of range [0, 5])` for a
  number outside 0 to 5.
- `bad argument #1 to 'setLevel' (number has no integer representation)` for
  a number such as 1.5.

The level is shared by the whole plugin, not only by scripts. It applies to
the plugin's own messages too, including the errors of event handlers and
timers: `"off"` hides those as well.

It is the same setting as `log.level` in `luaconfig.lua`. `Server.reload()`
sets it back to the value in the config file.

### Logger.getLevel()

Returns the current minimum level as a name: `"trace"`, `"debug"`, `"info"`,
`"warn"`, `"error"`, `"critical"` or `"off"`.

## print

`print(...)` writes one line at the `info` level. Its arguments are converted
with `tostring` and separated by tabs.

## Output format

Console lines look like `[14:03:07] [VCMP-Lua] [warning] text`. Log file lines
look like `[2026-10-09 14:03:07] [warning] text`. The level label of `warn`
messages is `warning`.

## Example

```lua
Logger.setLevel("debug")

Event.bind("onPlayerConnect", function(player)
    Logger.debug("connect: id " .. player.id)
end)

Event.bind("onPlayerCommand", function(player, command, args, text)
    if command == "login" and not args then
        Logger.warn(player.name .. " sent /login without a password")
    end
end)

Logger.info("commands loaded; log level is " .. Logger.getLevel())
```
