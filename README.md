# VCMP-Lua

A Lua 5.4 scripting plugin for the Vice City: Multiplayer (VC:MP) 0.4 server.

- **One file, nothing to install.** `LuaPlugin_x64.dll` (Windows) or
  `LuaPlugin_x64.so` (Linux, glibc 2.28 or newer). No runtime libraries,
  no DLLs next to it.
- **Batteries included.** SQLite, PostgreSQL and MySQL/MariaDB through
  LuaSQL, JSON through lua-cjson, LuaSocket, Copas, LuaFileSystem and
  inspect.lua are built in and load with `require`. The plugin adds a
  non-blocking HTTPS client (`http`), password hashing (`Hash`) and safe
  SQL values (`sql`).
- **Scripts cannot crash the server.** Every handle is checked before use,
  arguments are checked, errors are logged with a traceback, and the server
  owns entity lifetimes.

```lua
Event.bind("onPlayerCommand", function(player, command, args)
  if command == "car" then
    local pos = player.position
    local vehicle = Vehicle.create(tonumber(args and args[1]) or 191, player.world,
      pos[1] + 3, pos[2], pos[3], player.angle)
    player:msg("Your vehicle: " .. vehicle.id)
  end
end)
```

## Install

1. Download the zip for your server's platform from
   [Releases](https://github.com/rasikhq/VCMP-Lua/releases) and check it
   against `SHA256SUMS`.
2. Copy `LuaPlugin_x64.dll` or `LuaPlugin_x64.so` to the server's
   `plugins` directory, and add `LuaPlugin_x64` to the `plugins` line of
   `server.cfg`.
3. Copy `luaconfig.lua` to the server directory and list your scripts in
   it:

```lua
return {
  scripts = { "lua/main.lua" },
  package_path = "lua/?.lua;lua/?/init.lua",
  log = { level = "info" },
}
```

## Documentation

- [API reference](docs/api/README.md)
- [Configuration](docs/configuration.md)
- [Examples](examples/): commands, accounts with hashed passwords in
  SQLite, and a JSON webhook over HTTPS
- [Migrating from 2.x](docs/MIGRATION-v3.md)
- [Internals](docs/internals.md): how the server behaves, measured

## Building

The dependencies come from [vcpkg](https://vcpkg.io) (pinned in
`vcpkg-configuration.json`) and from release tarballs pinned by SHA256
(`cmake/deps/`).

### Linux

The Linux plugin is built in a pinned manylinux_2_28 image, so it loads on
older distributions too. With Docker:

```bash
docker build --platform linux/amd64 -t vcmp-lua-build -f ci/manylinux.Dockerfile ci
```

```bash
docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src vcmp-lua-build ci/build-linux.sh
```

The plugin is written to `build/linux-release/bin/LuaPlugin_x64.so`.

### Windows

With Visual Studio 2022 (C++ workload) and Git:

```powershell
ci/bootstrap-vcpkg.ps1 .cache/vcpkg
$env:VCPKG_ROOT = "$PWD/.cache/vcpkg"
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

The plugin is written to `build/windows-release/bin/LuaPlugin_x64.dll`.

## Tests

`ctest` runs the unit tests and loads the built plugin into a fake server
(`tests/host`). `ci/test-asan.sh` runs the unit tests under AddressSanitizer
and UndefinedBehaviorSanitizer. The integration smoke test runs the plugin
in the real VC:MP 0.4 Linux server against Postgres 17, MySQL 8.4,
MariaDB 11 and a local TLS server, with Docker; pass it the server
download:

```bash
tests/integration/run.sh /path/to/VCMP04_server_v46_linux64.zip
```

`tests/integration/bindings/run.sh` and `tests/integration/examples/run.sh`
take the same argument and run every binding, and the examples, in the real
server.

## Releases

Pushing a `v*` tag that matches the version in `CMakeLists.txt` builds and
tests both plugins, and drafts a GitHub release with one zip per platform
(plugin, `luaconfig.lua`, `LICENSE` and the third-party notices and
license texts) and `SHA256SUMS`.

## License

MIT, see [LICENSE](LICENSE). The bundled libraries are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), with their license texts in
[THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt).
