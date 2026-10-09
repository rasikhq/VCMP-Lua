#!/usr/bin/env bash
# Runs examples/ in a real VC:MP 0.4 Linux x64 server. driver.lua loads the
# example scripts and drives their command handlers with stand-in players.
# Build the plugin first (ci/build-linux.sh), then:
#
#   tests/integration/examples/run.sh /path/to/VCMP04_server_v46_linux64.zip
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
zip=${1:?usage: run.sh <VC:MP server zip>}
plugin="$root/build/linux-release/bin/LuaPlugin_x64.so"
[[ -f "$plugin" ]] || { echo "build the plugin first: $plugin is missing" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
unzip -q "$zip" -d "$work"
chmod +x "$work/mpsvrrel64"
mkdir -p "$work/plugins"
cp "$plugin" "$work/plugins/"
cp -r "$root/examples/lua" "$work/lua"
cp "$here/driver.lua" "$work/driver.lua"
printf 'gamemode examples\nport 5192\nmaxplayers 50\nplugins LuaPlugin_x64\n' > "$work/server.cfg"
# The example config, with driver.lua as its only script.
sed -e 's#"lua/accounts.lua",#"driver.lua",#' -e '/"lua\/commands.lua",/d' -e '/"lua\/webhook.lua",/d' \
    "$root/examples/luaconfig.lua" > "$work/luaconfig.lua"
chmod -R a+rwX "$work"

status=0
docker run --rm -t --platform linux/amd64 --user 1000:1000 -v "$work:/server" -w /server \
    ubuntu:24.04 timeout 30 ./mpsvrrel64 2>&1 | tee "$work/output.log" || status=$?
echo "server exit status: $status"
grep -q "EXAMPLES PASS" "$work/output.log" && [[ $status -eq 0 ]]
