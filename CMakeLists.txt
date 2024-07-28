cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0079 NEW)

include(FetchContent)
include(ExternalProject)

project(VCMPLua LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake options
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Static Build" FORCE)

# https://github.com/SRombauts/SQLiteCpp/issues/452
set(SQLITECPP_RUN_CPPLINT OFF)

file(GLOB_RECURSE 
	SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/pch.h
		${CMAKE_CURRENT_SOURCE_DIR}/Core.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/vcmpWrap/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/vcmpWrap/*.cpp
)

add_library(VCMPLua SHARED ${SOURCE_FILES})

target_include_directories(VCMPLua PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(VCMPLua PRIVATE ${CMAKE_SOURCE_DIR}/vcmpWrap)
target_include_directories(VCMPLua PRIVATE include)

target_compile_features(VCMPLua PRIVATE cxx_std_17)

# Dependencies [conan managed]
find_package(ZLIB REQUIRED)
find_package(sol2 REQUIRED)
find_package(Async++ REQUIRED)
find_package(cpr REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(libmysqlclient REQUIRED)

target_link_libraries(VCMPLua sol2::sol2)
target_link_libraries(VCMPLua Async++)
target_link_libraries(VCMPLua cpr::cpr)
target_link_libraries(VCMPLua spdlog::spdlog)
target_link_libraries(VCMPLua SQLiteCpp)
target_link_libraries(VCMPLua libmysqlclient::libmysqlclient)

# Dependencies [submodules]
add_subdirectory(${VENDOR_DIR}/SimpleINI)
add_subdirectory(${VENDOR_DIR}/digestpp)

add_subdirectory(${VENDOR_DIR}/lanes)
target_link_libraries(lanes lua::lua) # Available from sol2
target_link_libraries(VCMPLua lanes)

# Extra configuration
if(MSVC)
	target_compile_options(VCMPLua PRIVATE "/MP")
	target_compile_options(lanes PRIVATE "/MP")
endif()