# lua-cjson (OpenResty fork) 2.1.0.19: require "cjson" and "cjson.safe".
# The fork handles Lua 5.4 integers.
vcmp_lua_fetch(lua_cjson
    URL https://github.com/openresty/lua-cjson/archive/refs/tags/2.1.0.19.tar.gz
    SHA256 d1aded44b4cfe5ec6b395e178902aba3eed1dbe7999a753c0662222de2890ec0
    PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/lua-cjson-export-macro.patch")

# Same settings as upstream's CMakeLists.txt; libc number conversion (fpconv.c).
set(_cjson_definitions CJSON_EXPORT=)
if(WIN32)
    list(APPEND _cjson_definitions DISABLE_INVALID_NUMBERS)
endif()
if(MSVC)
    list(APPEND _cjson_definitions inline=__inline)
endif()

vcmp_lua_c_module(vcmp_lua_cjson
    SOURCES
        "${lua_cjson_SOURCE_DIR}/lua_cjson.c"
        "${lua_cjson_SOURCE_DIR}/strbuf.c"
        "${lua_cjson_SOURCE_DIR}/fpconv.c"
    INCLUDE_DIRS "${lua_cjson_SOURCE_DIR}"
    DEFINITIONS ${_cjson_definitions})
