cmake_minimum_required (VERSION 3.6)
project(VCMPLua)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(VCMP-LUA/vendor/asyncplusplus)
add_subdirectory(VCMP-LUA/vendor/spdlog)
add_subdirectory(VCMP-LUA/vendor/Lua)
add_subdirectory(VCMP-LUA/vendor/lanes)
# add_subdirectory(VCMP-LUA/vendor/mysql-connector-cpp)

add_subdirectory(VCMP-LUA)