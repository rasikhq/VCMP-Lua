# LuaFileSystem 1.9.0: require "lfs".
# Built here instead of from the vcpkg port, because lfs.h hard-codes
# __declspec(dllexport) on Windows (see the patch).
vcmp_lua_fetch(luafilesystem
    URL https://github.com/lunarmodules/luafilesystem/archive/refs/tags/v1_9_0.tar.gz
    SHA256 1142c1876e999b3e28d1c236bf21ffd9b023018e336ac25120fb5373aade1450
    PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/luafilesystem-export-macro.patch")

vcmp_lua_c_module(vcmp_lua_lfs
    SOURCES "${luafilesystem_SOURCE_DIR}/src/lfs.c"
    INCLUDE_DIRS "${luafilesystem_SOURCE_DIR}/src"
    DEFINITIONS LFS_EXPORT=)
