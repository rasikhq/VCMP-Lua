#!/usr/bin/env bash
# Builds, tests and checks the Linux plugin. Runs inside the pinned image
# (ci/manylinux.Dockerfile), in CI and locally:
#
#   docker build --platform linux/amd64 -t vcmp-lua-build -f ci/manylinux.Dockerfile ci
#   docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src vcmp-lua-build ci/build-linux.sh
#
# vcpkg and its binary cache live in .cache/ of the checkout.
set -euo pipefail
cd "$(dirname "$0")/.."

# The checkout belongs to another user than the container's root.
export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0='*'
export VCPKG_ROOT="${VCPKG_ROOT:-$PWD/.cache/vcpkg}"
export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-$PWD/.cache/vcpkg-archives}"
mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"
ci/bootstrap-vcpkg.sh "$VCPKG_ROOT"

cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
ci/check-binary.sh build/linux-release/bin/LuaPlugin_x64.so
