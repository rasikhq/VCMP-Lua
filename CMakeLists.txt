cmake_minimum_required(VERSION 3.25)

project(vcmp_lua
    VERSION 2.0.0
    DESCRIPTION "Lua 5.4 scripting plugin for the VC:MP 0.4 server"
    HOMEPAGE_URL "https://github.com/rasikhq/VCMP-Lua"
    LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "VCMP-Lua builds for x64 only.")
endif()

option(VCMP_LUA_BUILD_TESTS "Build the tests" ON)
option(VCMP_LUA_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
# Static CRT (/MT), as in the x64-windows-static-plugin triplet.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
if(MSVC)
    # /EHs, not /EHsc: Lua is compiled as C, and the panic handler throws a
    # C++ exception through its frames, so extern "C" calls may throw.
    string(REPLACE "/EHsc" "/EHs" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# Everything compiled here, third-party Lua modules included. The vcpkg
# libraries are not instrumented. Any finding fails the test (no recovery).
if(VCMP_LUA_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "VCMP_LUA_SANITIZE is for GCC and Clang")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all
                        -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(Hardening)
include(EmbedLua)
include(EmbedFile)

# Native libraries (vcpkg.json).
find_package(unofficial-lua CONFIG REQUIRED)
if(NOT TARGET lua)
    message(FATAL_ERROR "The vcpkg lua port no longer defines the imported target 'lua'.")
endif()
find_package(sol2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-libmariadb CONFIG REQUIRED)

# Lua modules and header-only libraries from pinned tarballs (cmake/deps).
include(deps/common)
include(deps/luafilesystem)
include(deps/lua-cjson)
include(deps/luasocket)
include(deps/luasql)
include(deps/copas)
include(deps/inspect)
include(deps/digestpp)
if(NOT WIN32)
    include(deps/cacert)
endif()

add_subdirectory(src)

if(VCMP_LUA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit)
    add_subdirectory(tests/host)
endif()
