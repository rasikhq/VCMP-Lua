#pragma once

#include <string>

namespace vcmp_lua::libraries {

// Process-wide set-up of the C libraries the plugin links. Must run before
// any of them is used: OpenSSL is initialised first, without an atexit
// handler and without reading openssl.cnf. Throws on failure.
void Init();

// Process-wide clean-up once no runtime exists anymore (OnServerShutdown).
// Never called from a static destructor or DllMain.
void Cleanup() noexcept;

// "Lua 5.4.8, sol2 3.5.0, ..." for the start-up log.
std::string Versions();

}  // namespace vcmp_lua::libraries
