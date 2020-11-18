cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "VCMP-Lua")

project(${PROJECT_NAME} VERSION 1.9 LANGUAGES CXX)

option(BUILD_64 "" ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(BUILD_64)
	set(LUA_PLATFORM "x64")
else()
	set(LUA_PLATFORM "x32")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_subdirectory(VCMP-LUA)