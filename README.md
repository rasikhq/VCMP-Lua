# VCMP-Lua

A Lua 5.4 scripting plugin for the Vice City: Multiplayer (VC:MP) 0.4 server.

> **v2 is in development on this branch.** The plugin binary, the build and the
> runtime are being rebuilt; the scripting API returns in a later phase. Use
> the `master` branch (v1) for servers today.

The plugin is one self-contained x64 binary per platform,
`LuaPlugin_x64.so` (Linux, glibc 2.28 or newer) and `LuaPlugin_x64.dll`
(Windows). It needs nothing else installed: Lua, LuaSQL (SQLite, PostgreSQL,
MySQL/MariaDB), lua-cjson, LuaSocket, Copas, LuaFileSystem and inspect.lua
are built in and load with `require`, next to the plugin's own `http`
(non-blocking HTTPS), `Hash` and `sql` modules.

## Configuration

Put `luaconfig.lua` in the server directory:

```lua
return {
  scripts = { "lua/main.lua" },
  package_path = "lua/?.lua;lua/?/init.lua",
  log = { level = "info" },
  -- CA certificates (PEM) for require "http". Default: the Windows
  -- certificate store; on Linux the distribution's bundle, else a built-in
  -- copy of Mozilla's.
  http = { cafile = nil },
}
```

## Building

The dependencies come from [vcpkg](https://vcpkg.io) (pinned in
`vcpkg-configuration.json`) and from release tarballs pinned by SHA256
(`cmake/deps/`).

### Linux

The Linux plugin is built in a pinned manylinux_2_28 image, so it loads on
older distributions too. With Docker:

```bash
docker build --platform linux/amd64 -t vcmp-lua-build -f ci/manylinux.Dockerfile ci
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
(`tests/host`). The integration smoke test runs the plugin in the real
VC:MP 0.4 Linux server against Postgres 17, MySQL 8.4, MariaDB 11 and a
local TLS server, with Docker; pass it the server download:

```bash
tests/integration/run.sh /path/to/VCMP04_server_v46_linux64.zip
```

## License

MIT, see [LICENSE](LICENSE). The bundled libraries are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
