#!/usr/bin/env bash
# Puts vcpkg at the baseline pinned in vcpkg-configuration.json into the
# directory given as $1 (cloning it the first time) and builds the tool.
set -euo pipefail

root="$1"
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
baseline="$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["default-registry"]["baseline"])' \
    "$repo_dir/vcpkg-configuration.json")"

if [ ! -d "$root/.git" ]; then
    git clone https://github.com/microsoft/vcpkg "$root"
fi
if [ "$(git -C "$root" rev-parse HEAD)" != "$baseline" ] || [ ! -x "$root/vcpkg" ]; then
    git -C "$root" fetch origin
    git -C "$root" checkout --quiet --detach "$baseline"
    "$root/bootstrap-vcpkg.sh" -disableMetrics
fi
