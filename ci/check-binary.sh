#!/usr/bin/env bash
# Checks the Linux plugin against the single-binary rules:
#   - it needs only glibc's own libraries (no libstdc++, OpenSSL, ...);
#   - VcmpPluginInit is its only exported symbol;
#   - it needs no glibc symbol version newer than 2.28;
#   - it is hardened: no text relocations, full RELRO, non-executable stack.
#
# usage: ci/check-binary.sh build/linux-release/bin/LuaPlugin_x64.so
set -euo pipefail

so="$1"
max_glibc="2.28"
failed=0

fail() {
    echo "check-binary: $*" >&2
    failed=1
}

allowed_needed='^(libc\.so\.6|libm\.so\.6|libpthread\.so\.0|libdl\.so\.2|librt\.so\.1|ld-linux-x86-64\.so\.2)$'
needed="$(readelf --dynamic --wide "$so" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p')"
for lib in $needed; do
    if [[ ! "$lib" =~ $allowed_needed ]]; then
        fail "needs $lib, which is not part of glibc"
    fi
done

exports="$(nm --dynamic --defined-only "$so" | awk 'NF == 3 { print $3 }' | sort -u | tr '\n' ' ')"
if [ "$exports" != "VcmpPluginInit " ]; then
    fail "exports must be exactly VcmpPluginInit, got: $exports"
fi

newest_glibc="$(objdump --dynamic-syms "$so" | grep -o 'GLIBC_[0-9][0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -n 1)"
if [ -n "$newest_glibc" ] && [ "$(printf '%s\n%s\n' "$newest_glibc" "$max_glibc" | sort -V | tail -n 1)" != "$max_glibc" ]; then
    fail "needs GLIBC_$newest_glibc, newer than GLIBC_$max_glibc"
fi

dynamic="$(readelf --dynamic --wide "$so")"
if grep -q 'TEXTREL' <<<"$dynamic"; then
    fail "has text relocations"
fi
if ! grep -Eq 'BIND_NOW|FLAGS.*NOW' <<<"$dynamic"; then
    fail "is not linked with -z now"
fi
if ! readelf --program-headers --wide "$so" | grep -q 'GNU_RELRO'; then
    fail "has no RELRO segment"
fi
if readelf --program-headers --wide "$so" | grep 'GNU_STACK' | grep -q 'RWE'; then
    fail "has an executable stack"
fi

echo "check-binary: $so"
echo "  NEEDED:  $(echo $needed)"
echo "  exports: $exports"
echo "  newest glibc symbol version: GLIBC_${newest_glibc:-none}"
if [ "$failed" -ne 0 ]; then
    echo "check-binary: FAILED" >&2
    exit 1
fi
echo "check-binary: OK"
