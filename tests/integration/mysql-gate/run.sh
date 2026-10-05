#!/usr/bin/env bash
# MySQL gate (plan, Part C, P1). Runs inside the vcmp-lua-build image next to
# MySQL 8.4 and MariaDB 11; see tests/integration/compose.yml. Uses the Linux
# build in build/linux-release.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
build=/src/build/linux-release
work=/tmp/mysql-gate
rm -rf "$work"
mkdir -p "$work/run" "$work/env-plugins" "$work/plugins/libmariadb"

# Fake client plugins wherever MariaDB Connector/C could look for plugins:
# $MARIADB_PLUGIN_DIR and its built-in default, ../plugins/libmariadb
# relative to the working directory.
for dir in "$work/env-plugins" "$work/plugins/libmariadb"; do
    for name in caching_sha2_password client_ed25519 dialog mysql_clear_password \
                mysql_native_password sha256_password zstd; do
        gcc -shared -fPIC -o "$dir/$name.so" "-DMARKER=\"$dir/$name.loaded\"" "$here/planted_plugin.c"
    done
done
export MARIADB_PLUGIN_DIR="$work/env-plugins"

cp "$here/gate.lua" "$work/run/"
printf 'return { scripts = { "gate.lua" } }\n' > "$work/run/luaconfig.lua"
cd "$work/run"
status=0
"$build/tests/host/plugin_host" "$build/bin/LuaPlugin_x64.so" --frames 1 --expect mysql-gate.result || status=1

loaded="$(find "$work" -name '*.loaded')"
if [ -n "$loaded" ]; then
    echo "client plugins were loaded from disk:"
    echo "$loaded"
    status=1
fi
if [ "$status" -ne 0 ]; then
    echo "MYSQL GATE FAIL"
    exit 1
fi
echo "MYSQL GATE PASS"
