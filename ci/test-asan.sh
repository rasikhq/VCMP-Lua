#!/usr/bin/env bash
# Builds the unit tests with AddressSanitizer and UndefinedBehaviorSanitizer
# and runs them (preset linux-asan). Any finding fails. Runs inside the pinned
# image, like ci/build-linux.sh:
#
#   docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src vcmp-lua-build ci/test-asan.sh
set -euo pipefail
cd "$(dirname "$0")/.."

export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0='*'
export VCPKG_ROOT="${VCPKG_ROOT:-$PWD/.cache/vcpkg}"
export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-$PWD/.cache/vcpkg-archives}"
mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"
ci/bootstrap-vcpkg.sh "$VCPKG_ROOT"

cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
