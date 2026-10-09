#!/usr/bin/env bash
# Runs the integration smoke test (tests/integration/compose.yml) with the
# real VC:MP 0.4 Linux x64 server. Build the plugin first (ci/build-linux.sh),
# then:
#
#   tests/integration/run.sh /path/to/VCMP04_server_v46_linux64.zip
#
# Exits 0 when smoke.lua printed SMOKE PASS and the server shut down cleanly.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
zip=${1:?usage: run.sh <VC:MP server zip>}
[[ -f "$root/build/linux-release/bin/LuaPlugin_x64.so" ]] ||
    { echo "build the plugin first (ci/build-linux.sh)" >&2; exit 1; }

server=$(mktemp -d)
compose=(docker compose -f "$here/compose.yml")
cleanup() {
    "${compose[@]}" down --volumes >/dev/null 2>&1 || true
    rm -rf "$server"
}
trap cleanup EXIT
unzip -q "$zip" -d "$server"
chmod -R a+rX "$server"

status=0
VCMP_SERVER_DIR="$server" "${compose[@]}" up --exit-code-from vcmp || status=$?
echo "smoke test exit status: $status"
exit "$status"
