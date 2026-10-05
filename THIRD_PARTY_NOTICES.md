# Third-party notices

The VCMP-Lua plugin binary contains the following software, linked statically.
Versions are pinned in `vcpkg.json`, `vcpkg-configuration.json` and
`cmake/deps/`.

| Component | Version | License | Source |
|---|---|---|---|
| Lua | 5.4.8 | MIT | https://www.lua.org |
| sol2 | 3.5.0 | MIT | https://github.com/ThePhD/sol2 |
| spdlog | 1.17.0 | MIT | https://github.com/gabime/spdlog |
| {fmt} | 12.2.0 | MIT | https://github.com/fmtlib/fmt |
| SQLite | 3.53.4 | Public domain | https://sqlite.org |
| OpenSSL | 3.6.5 | Apache-2.0 | https://www.openssl.org |
| libpq (PostgreSQL) | 18.4 | PostgreSQL License | https://www.postgresql.org |
| MariaDB Connector/C | 3.4.11 | LGPL-2.1-or-later | https://github.com/mariadb-corporation/mariadb-connector-c |
| libcurl | 8.22.0 | curl License | https://curl.se |
| zlib | 1.3.2 | zlib License | https://zlib.net |
| LuaFileSystem | 1.9.0 | MIT | https://github.com/lunarmodules/luafilesystem |
| lua-cjson (OpenResty) | 2.1.0.19 | MIT | https://github.com/openresty/lua-cjson |
| LuaSocket | 3.1.0 | MIT | https://github.com/lunarmodules/luasocket |
| LuaSQL | 2.8.1 | MIT | https://github.com/lunarmodules/luasql |
| Copas | 4.12.0 | MIT | https://github.com/lunarmodules/copas |
| binaryheap.lua | 0.4 | MIT | https://github.com/Tieske/binaryheap.lua |
| timerwheel.lua | 1.0.2 | MIT | https://github.com/Tieske/timerwheel.lua |
| inspect.lua | 3.1.3 | MIT | https://github.com/kikito/inspect.lua |
| digestpp | 4beae75 | Unlicense (public domain) | https://github.com/kerukuro/digestpp |
| VC:MP 0.4 plugin SDK header | 0.4 | Apache-2.0 | `third_party/vcmp/vcmp.h` |

MariaDB Connector/C is licensed under the LGPL. VCMP-Lua links it statically;
the complete source of VCMP-Lua and the build scripts that produce the binary
are public in this repository, so the plugin can be rebuilt against a
modified copy of the library.

VCMP-Lua patches some of these components at build time; the patches are in
`cmake/deps/patches/`.

The full license texts are collected into this file before the first v2
release.
