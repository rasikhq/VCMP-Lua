cmake_minimum_required (VERSION 3.6)
project(VCMPLua)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(VCMP-LUA/vendor/asyncplusplus)
add_subdirectory(VCMP-LUA)