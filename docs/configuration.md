# Configuration

The plugin reads its settings from `luaconfig.lua`. The file is a Lua script
that returns a table. It is required: without a valid config file the plugin
does not load.

## Location

The plugin opens `luaconfig.lua` in the server's working directory, which is
normally the directory of the server executable. Relative paths inside the
file (scripts, log file, CA file, `package_path`) are also relative to the
working directory.

## Example

```lua
return {
  -- Run in this order when the server starts and on Server.reload().
  scripts = {
    "lua/commands.lua",
    "lua/accounts.lua",
  },

  -- Where require looks for your own modules.
  package_path = "lua/?.lua;lua/?/init.lua",

  log = {
    level = "info",
    file = "logs/vcmp-lua.log",
    daily = true,
  },

  http = {
    cafile = "certs/ca.pem",
  },
}
```

## Settings

Every setting is optional. A setting that is left out, or set to `nil`, keeps
its default.

| Setting | Type | Default | Meaning |
|---|---|---|---|
| `scripts` | list of strings | `{}` | Script files to run, in order. |
| `package_path` | string | `"lua/?.lua;lua/?/init.lua"` | The value of `package.path`. |
| `log.level` | string | `"info"` | The minimum level of the plugin log. |
| `log.file` | string | none | Also write the log to this file. |
| `log.daily` | boolean | `false` | Start a new log file every day. |
| `http.cafile` | string | none | CA certificates (PEM) for `require "http"`. |

### scripts

The files run in the order listed, each in the same Lua state. Only the
array part of the table is read (`scripts[1]` to `scripts[#scripts]`).

- Scripts must be Lua source text. Precompiled bytecode is refused.
- A script that fails to load or raises an error is logged, and the next
  script still runs.
- After the last script, `onServerInit` fires.

The scripts run when the server initialises, not when the plugin is loaded.

### package_path

Replaces `package.path`, so `require "foo"` finds your own modules. The
built-in modules (`http`, `cjson`, `luasql.*` and the others) load
regardless of this setting. `package.cpath` is not changed.

### log

`log` is a table with these fields:

- `level`: one of `"trace"`, `"debug"`, `"info"`, `"warn"`, `"error"`,
  `"critical"`, `"off"`. Messages below this level are not written. Scripts
  can change it at run time with [`Logger.setLevel`](api/logger.md).
- `file`: a path. The plugin log is written to this file as well as to the
  console. The file is appended to, not truncated. Missing directories are
  created. Each line is written to the file at once.
- `daily`: with `true`, the file name gets the date, and a new file starts at
  midnight local time: `logs/vcmp-lua.log` becomes
  `logs/vcmp-lua_2026-10-09.log`. Old files are kept. Without `file`, `daily`
  has no effect.

If the log file cannot be opened, the plugin logs
`cannot open the log file <file>: <reason>` and keeps logging to the console.

### http

`http` is a table with one field:

- `cafile`: a PEM file of CA certificates that `require "http"` trusts
  instead of the defaults. The defaults are the Windows certificate store on
  Windows; on Linux, the distribution's CA bundle, or a built-in copy of
  Mozilla's bundle if there is none.

## Sandbox

The config file runs in its own Lua state, separate from the scripts.

- Available: the base functions, `string`, `table`, `math` and `utf8`.
- Not available: `io`, `os`, `require` and `package`, `debug`, `coroutine`,
  and the base functions `dofile`, `loadfile`, `load` and `collectgarbage`.
- `print` works, but writes straight to standard output, not to the plugin
  log.
- The file may run at most 10,000,000 Lua VM instructions. Beyond that it
  fails with `the config file runs too long (endless loop?)`.
- The file must be Lua source text. Precompiled bytecode is refused.

## Validation

The returned table is checked field by field. The first error stops the load.
Errors name the file and the field:

```
luaconfig.lua must return a table (got number)
luaconfig.lua: scripts must be a list of strings (got string)
luaconfig.lua: scripts[2] must be a string (got number)
luaconfig.lua: log must be a table (got boolean)
luaconfig.lua: log.daily must be a boolean (got string)
luaconfig.lua: log.level must be one of trace, debug, info, warn, error, critical, off (got "loud")
```

A syntax error or a runtime error in the file is reported with Lua's own
message.

Unknown settings are not errors. They are logged as warnings and ignored:

```
luaconfig.lua: unknown setting 'logs' is ignored
luaconfig.lua: unknown setting 'log.colour' is ignored
```

A key that is not a string (for example a list entry at the top level) is
also ignored with a warning.

## When the file is read

- **Plugin load.** The plugin reads `luaconfig.lua` while the server loads
  it, before any script runs. If the file is missing or invalid,
  the plugin logs `VcmpPluginInit: <error>` and does not load. Warnings and
  errors from this first read go to the console only, because the log file
  is opened after the config is read.
- **`Server.reload()`.** At the end of the frame the plugin reads the file
  again. If it is now invalid, the plugin logs `Server.reload: <error>` and
  `Server.reload: the scripts keep running unchanged`; nothing changes.
  Otherwise every setting takes effect again: the new script list runs in a
  fresh Lua state, `package_path` and `http.cafile` apply to it, the log level
  is reset to `log.level` (undoing `Logger.setLevel`), and the log file is
  closed and reopened from `log.file` and `log.daily`. Removing `log.file`
  stops writing to a file.

The file is not watched for changes. Edits take effect only through
`Server.reload()` or a server restart.
