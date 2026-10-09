# Static, release-only libraries that are linked into the Linux plugin (.so).
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)

# Every library ends up inside one shared object. Hidden visibility keeps
# their symbols out of the dynamic symbol table; cmake/plugin.map enforces it.
set(VCPKG_C_FLAGS "-fPIC -fvisibility=hidden")
set(VCPKG_CXX_FLAGS "-fPIC -fvisibility=hidden -fvisibility-inlines-hidden")

# OpenSSL: never pin the plugin in memory (it must be able to unload) and
# never read openssl.cnf from the build machine's OPENSSLDIR.
set(OPENSSL_USE_NOPINSHARED ON)
set(OPENSSL_NO_AUTOLOAD_CONFIG ON)
