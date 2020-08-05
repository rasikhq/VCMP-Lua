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
	${PROJECT_SOURCE_DIR}/Core.cpp
	${PROJECT_SOURCE_DIR}/pch.cpp
	# include
	${PROJECT_SOURCE_DIR}/include/SimpleIni.h
	${PROJECT_SOURCE_DIR}/include/utility.h
	${PROJECT_SOURCE_DIR}/include/vcmp.h
	# sol
	${PROJECT_SOURCE_DIR}/sol/forward.hpp
	${PROJECT_SOURCE_DIR}/sol/sol.hpp
	# vcmpWrap
	${PROJECT_SOURCE_DIR}/vcmpWrap/globalTables.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/vcmpCallbacks.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Player.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Player.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Vehicle.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Vehicle.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Server.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Server.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Stream.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Classes/Stream.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/EventManager/EventManager.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/EventManager/EventManager.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/Timer/vcmpTimer.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Timer/vcmpTimer.cpp
	${PROJECT_SOURCE_DIR}/vcmpWrap/Timer/TimerManager.h
	${PROJECT_SOURCE_DIR}/vcmpWrap/Timer/TimerManager.cpp
	# Lua
	${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/lauxlib.h
	${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/lua.h
	${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/lua.hpp
	${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/luaconf.h
	${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/lualib.h
)

target_include_directories(lib-lua
	PRIVATE
		${PROJECT_SOURCE_DIR}/
		${PROJECT_SOURCE_DIR}/include/
		${PROJECT_SOURCE_DIR}/vcmpWrap/
)

target_include_directories(lib-lua PRIVATE ${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/include/)
target_link_libraries(lib-lua ${PROJECT_SOURCE_DIR}/lua/${LUA_PLATFORM}/lua53.lib)

set_target_properties(lib-lua PROPERTIES PREFIX "")
set_target_properties(lib-lua PROPERTIES OUTPUT_NAME "lua-plugin-${LUA_PLATFORM}")