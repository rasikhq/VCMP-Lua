# Static, release-only libraries with the static CRT (/MT) that are linked
# into the Windows plugin (.dll), so it needs no VC++ redistributable.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)

# OpenSSL: never pin the plugin in memory (it must be able to unload) and
# never read openssl.cnf from the build machine's OPENSSLDIR.
set(OPENSSL_USE_NOPINSHARED ON)
set(OPENSSL_NO_AUTOLOAD_CONFIG ON)
