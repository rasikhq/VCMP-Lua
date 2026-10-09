#!/usr/bin/env bash
# Runs smoke.lua in the real VC:MP server. The vcmp service of
# tests/integration/compose.yml runs this in ubuntu:24.04 as uid 1000, with
# the unzipped server in /server-dist, the repository in /src and the test
# CA in /certs. Exits 0 when the script printed SMOKE PASS, its finalizer ran
# at shutdown, and the server exited by itself with status 0.
set -euo pipefail

if [[ ! -f /server-dist/mpsvrrel64 ]]; then
    echo "SMOKE FAIL: set VCMP_SERVER_DIR to the unzipped VC:MP 0.4 Linux x64 server" >&2
    exit 1
fi

work=/tmp/vcmp
rm -rf "$work"
mkdir -p "$work/plugins"
cp -r /server-dist/. "$work/"
chmod +x "$work/mpsvrrel64"
cp /src/build/linux-release/bin/LuaPlugin_x64.so "$work/plugins/"
cp /src/tests/integration/smoke/smoke.lua "$work/"
printf 'gamemode smoke\nport 5192\nmaxplayers 10\nplugins LuaPlugin_x64\n' > "$work/server.cfg"
printf 'return { scripts = { "smoke.lua" }, log = { level = "debug" } }\n' > "$work/luaconfig.lua"

cd "$work"
status=0
timeout 300 ./mpsvrrel64 2>&1 | tee output.log || status=$?
echo "server exit status: $status"
grep -q "SMOKE PASS" output.log || { echo "SMOKE FAIL: no SMOKE PASS"; exit 1; }
grep -q "smoke finalizer ran" output.log || { echo "SMOKE FAIL: the finalizer did not run"; exit 1; }
[[ $status -eq 0 ]] || { echo "SMOKE FAIL: the server exited with $status"; exit 1; }
echo "SMOKE OK"
