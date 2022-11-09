cmake_minimum_required(VERSION 3.25)
project(VCMPLua LANGUAGES CXX)
set_property(GLOBAL PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY CXX_STANDARD 17)

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

target_include_directories(VCMPLua PRIVATE vendor)

get_directories(VENDORS ${VENDOR_DIR})
foreach(VENDOR ${VENDORS})
	message(STATUS "Found vendor: " ${VENDOR})
	add_subdirectory(${VENDOR_DIR}/${VENDOR})
endforeach()

foreach(VENDOR ${VENDORS})
	get_all_targets(_targets ${VENDOR_DIR}/${VENDOR})
	list(LENGTH _targets _total_targets)
	if(_total_targets EQUAL 1)
		get_target_property(_target_type ${_targets} TYPE)
		if(NOT _target_type STREQUAL "MODULE_LIBRARY")
			message(STATUS "Auto-Linking vendor... " ${_targets})
			target_link_libraries(VCMPLua PRIVATE ${_targets})
		endif()
	endif()
endforeach()

# Vendors that produce more than 1 target have to be manually linked
link_vendor(spdlog::spdlog)

# Manual inclusion of headers