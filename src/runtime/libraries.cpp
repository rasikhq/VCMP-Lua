#include "runtime/libraries.hpp"

#include <curl/curl.h>
#include <digestpp.hpp>
#include <fmt/format.h>
#include <libpq-fe.h>
#include <lua.hpp>
#include <mysql.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <sol/version.hpp>
#include <spdlog/version.h>
#include <sqlite3.h>
#include <zlib.h>

#include <stdexcept>

namespace vcmp_lua::libraries {
namespace {

struct Initialised {
    bool openssl = false;
    bool curl = false;
    bool mysql = false;
};
Initialised g_initialised;

// digestpp is header-only: a known-answer test shows it builds and runs here.
void CheckDigestpp() {
    const std::string digest = digestpp::sha256().absorb("abc").hexdigest();
    if (digest != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
        throw std::runtime_error("digestpp self-test failed");
    }
}

}  // namespace

void Init() {
    if (!g_initialised.openssl) {
        // First, before libcurl, libpq or MariaDB Connector/C can initialise
        // OpenSSL with defaults: no atexit handler into this module (it may be
        // unloaded) and no openssl.cnf from the build machine's OPENSSLDIR.
        if (OPENSSL_init_ssl(OPENSSL_INIT_NO_ATEXIT | OPENSSL_INIT_NO_LOAD_CONFIG, nullptr) != 1) {
            throw std::runtime_error("OpenSSL initialisation failed");
        }
        g_initialised.openssl = true;
    }
    if (!g_initialised.curl) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            throw std::runtime_error("libcurl initialisation failed");
        }
        g_initialised.curl = true;
    }
    if (!g_initialised.mysql) {
        // LuaSQL is built with LUASQL_MYSQL_NO_LIBRARY_END: the plugin owns
        // the client library's lifetime.
        if (mysql_library_init(0, nullptr, nullptr) != 0) {
            throw std::runtime_error("MariaDB Connector/C initialisation failed");
        }
        g_initialised.mysql = true;
    }
    CheckDigestpp();
}

void Cleanup() noexcept {
    if (g_initialised.mysql) {
        mysql_library_end();
        g_initialised.mysql = false;
    }
    if (g_initialised.curl) {
        curl_global_cleanup();
        g_initialised.curl = false;
    }
    sqlite3_shutdown();
    if (g_initialised.openssl) {
        // Required with OPENSSL_INIT_NO_ATEXIT. OpenSSL cannot be initialised
        // again in this process afterwards.
        OPENSSL_cleanup();
        g_initialised.openssl = false;
    }
}

std::string Versions() {
    const curl_version_info_data* curl = curl_version_info(CURLVERSION_NOW);
    const int pq = PQlibVersion();
    return fmt::format(
        "{}, sol2 {}, spdlog {}.{}.{}, fmt {}.{}.{}, {}, libcurl {} ({}), SQLite {}, "
        "libpq {}.{}, MariaDB Connector/C {}, zlib {}",
        LUA_RELEASE, SOL_VERSION_STRING, SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH,
        FMT_VERSION / 10000, FMT_VERSION / 100 % 100, FMT_VERSION % 100,
        OpenSSL_version(OPENSSL_VERSION), curl->version,
        curl->ssl_version != nullptr ? curl->ssl_version : "no TLS", sqlite3_libversion(),
        pq / 10000, pq % 100, mysql_get_client_info(), zlibVersion());
}

}  // namespace vcmp_lua::libraries
