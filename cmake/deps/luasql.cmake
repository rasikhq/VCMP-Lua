# LuaSQL 2.8.1: luasql.sqlite3, luasql.postgres and luasql.mysql.
# luasql.c (shared helpers) is compiled once for the three drivers.
vcmp_lua_fetch(luasql
    URL https://github.com/lunarmodules/luasql/archive/refs/tags/2.8.1.tar.gz
    SHA256 4c5c890b53c5c085329e6d152f1f711c4bbc79234f85ed4371646802c91093a4
    PATCHES
        "${CMAKE_CURRENT_LIST_DIR}/patches/luasql-mysql-safety.patch"
        "${CMAKE_CURRENT_LIST_DIR}/patches/luasql-mysql-options.patch")

set(_luasql_src "${luasql_SOURCE_DIR}/src")
# A plugin directory that cannot exist (see luasql-mysql-options.patch): a
# path below /dev/null on POSIX, characters Windows forbids in file names.
if(WIN32)
    set(_luasql_no_plugin_dir "|vcmp-lua-no-client-plugins|")
else()
    set(_luasql_no_plugin_dir "/dev/null/vcmp-lua-no-client-plugins")
endif()
vcmp_lua_c_module(vcmp_lua_luasql
    SOURCES
        "${_luasql_src}/luasql.c"
        "${_luasql_src}/ls_sqlite3.c"
        "${_luasql_src}/ls_postgres.c"
        "${_luasql_src}/ls_mysql.c"
    INCLUDE_DIRS "${_luasql_src}"
    # The plugin owns the MySQL client library's lifetime (src/plugin/entry.cpp).
    DEFINITIONS
        LUASQL_API=
        LUASQL_VERSION_NUMBER="2.8.1"
        LUASQL_MYSQL_NO_LIBRARY_END
        "LUASQL_MYSQL_PLUGIN_DIR=\"${_luasql_no_plugin_dir}\""
    LIBRARIES
        unofficial::sqlite3::sqlite3
        PostgreSQL::PostgreSQL
        unofficial::libmariadb)
