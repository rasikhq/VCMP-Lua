cmake_minimum_required(VERSION 3.10)

project(lib-lua VERSION 1.0 LANGUAGES CXX)

option(BUILD_64 "" ON)

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

add_library(lib-lua SHARED 
	Core.cpp
	pch.h
	pch.cpp
	# include
	include/SimpleIni.h
	include/utility.h
	include/vcmp.h
	# sol
	sol/forward.hpp
	sol/sol.hpp
	# vcmpWrap
	vcmpWrap/globalTables.h
	vcmpWrap/vcmpCallbacks.h
	vcmpWrap/Log/Logger.h
	vcmpWrap/Classes/Player.h
	vcmpWrap/Classes/Player.cpp
	vcmpWrap/Classes/Vehicle.h
	vcmpWrap/Classes/Vehicle.cpp
	vcmpWrap/Classes/Server.h
	vcmpWrap/Classes/Server.cpp
	vcmpWrap/Classes/Stream.h
	vcmpWrap/Classes/Stream.cpp
	vcmpWrap/EventManager/EventManager.h
	vcmpWrap/EventManager/EventManager.cpp
	vcmpWrap/Timer/vcmpTimer.h
	vcmpWrap/Timer/vcmpTimer.cpp
	vcmpWrap/Timer/TimerManager.h
	vcmpWrap/Timer/TimerManager.cpp
	# Lua
	lua/lauxlib.h
	lua/lua.h
	lua/lua.hpp
	lua/luaconf.h
	lua/lualib.h
)

target_include_directories(lib-lua
	PRIVATE
		${PROJECT_SOURCE_DIR}/
		include/
		vcmpWrap/
		lua/
		vendor/
)

add_subdirectory(lua)
target_link_libraries(lib-lua lualib)

set_target_properties(lib-lua PROPERTIES PREFIX "")
set_target_properties(lib-lua PROPERTIES OUTPUT_NAME "lua-plugin-${LUA_PLATFORM}")