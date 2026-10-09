#!/usr/bin/env bash
# Runs bindings.lua in a real VC:MP 0.4 Linux x64 server; no players needed.
# Build the plugin first (ci/build-linux.sh), then:
#
#   tests/integration/bindings/run.sh /path/to/VCMP04_server_v46_linux64.zip [script.lua]
#
# script.lua (default: bindings.lua next to this file) must print
# BINDINGS PASS and shut the server down.
# The server runs in ubuntu:24.04 (it needs glibc 2.38) as uid 1000 (it
# refuses root). Exits 0 when the script printed BINDINGS PASS and the
# server shut down by itself.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
zip=${1:?usage: run.sh <VC:MP server zip> [script.lua]}
script=${2:-$here/bindings.lua}
plugin="$root/build/linux-release/bin/LuaPlugin_x64.so"
[[ -f "$plugin" ]] || { echo "build the plugin first: $plugin is missing" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
unzip -q "$zip" -d "$work"
chmod +x "$work/mpsvrrel64"
mkdir -p "$work/plugins"
cp "$plugin" "$work/plugins/"
cp "$script" "$work/bindings.lua"
printf 'gamemode bindings\nport 5192\nmaxplayers ${MAXPLAYERS:-50}\nplugins LuaPlugin_x64\n' > "$work/server.cfg"
printf 'return { scripts = { "bindings.lua" } }\n' > "$work/luaconfig.lua"
chmod -R a+rwX "$work"

status=0
docker run --rm -t --platform linux/amd64 --user 1000:1000 -v "$work:/server" -w /server \
    ubuntu:24.04 timeout 20 ./mpsvrrel64 2>&1 | tee "$work/output.log" || status=$?
echo "server exit status: $status"
grep -q "BINDINGS PASS" "$work/output.log" && [[ $status -eq 0 ]]
