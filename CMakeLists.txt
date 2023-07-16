cmake_minimum_required(VERSION 3.2)
project(VCMPLua LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake options
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Static Build" FORCE)

message(STATUS "VENDOR_DIR: " ${VENDOR_DIR})

# Project options
add_compile_definitions(LIBASYNC_STATIC)

macro(get_directories result curdir)
	file(GLOB children RELATIVE ${curdir} ${curdir}/*)
	set(dirlist "")
	foreach(child ${children})
		if(IS_DIRECTORY ${curdir}/${child})
			list(APPEND dirlist ${child})
		endif()
	endforeach()
	set(${result} ${dirlist})
endmacro()

function(get_all_targets out_var subdirs)
    get_property(targets DIRECTORY ${subdirs} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${subdirs} PROPERTY SUBDIRECTORIES)

    foreach(subdir ${subdirs})
        get_all_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()

function(link_vendor _target)
	message(STATUS "Linking vendor... " ${_target})
	target_link_libraries(VCMPLua PRIVATE ${_target})
endfunction()

file(GLOB_RECURSE 
	SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/pch.h
		${CMAKE_CURRENT_SOURCE_DIR}/Core.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/vcmpWrap/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/vcmpWrap/*.cpp
)

add_library(VCMPLua SHARED ${SOURCE_FILES})

target_compile_features(VCMPLua PRIVATE cxx_std_17)
# target_precompile_headers(VCMPLua PUBLIC pch.h)

target_include_directories(VCMPLua PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(VCMPLua PRIVATE ${CMAKE_SOURCE_DIR}/vcmpWrap)
target_include_directories(VCMPLua PRIVATE include)

get_directories(VENDORS ${VENDOR_DIR})
foreach(VENDOR ${VENDORS})
	message(STATUS "Found vendor: " ${VENDOR})
	add_subdirectory(${VENDOR_DIR}/${VENDOR})
endforeach()

foreach(VENDOR ${VENDORS})
	get_all_targets(_targets ${VENDOR_DIR}/${VENDOR})
	list(LENGTH _targets _total_targets)
	message(STATUS ${VENDOR} " has " ${_total_targets} " targets")
	if(_total_targets GREATER_EQUAL 1)
		list(GET _targets 0 _main_target)
		get_target_property(_target_type ${_main_target} TYPE)
		if((NOT _main_target STREQUAL "MODULE_LIBRARY") AND (NOT _main_target STREQUAL "uninstall"))
			message(STATUS "Auto-Linking vendor... " ${_main_target})
			target_link_libraries(VCMPLua PRIVATE ${_main_target})
		endif()
	else()
		message(NOTICE "[!] Skip linking " ${VENDOR} " as it has no targets to link to (header-only libs)")
	endif()
endforeach()

# Vendors that produce more than 1 target have to be manually linked
link_vendor(cpr::cpr)
# link_vendor(lua::lib)
# link_vendor(spdlog::spdlog)
# link_vendor(SQLiteCpp)