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

	# Fix for mariadb-connector-c
	# https://stackoverflow.com/questions/66391677/cmake-builds-when-in-directory-but-it-wont-when-add-subdirectory-is-used
	if(${VENDOR} STREQUAL "mariadb-connector-c")
		message(STATUS "Applying fix for amriadb-connector-c as an add_subdirectory")
		set(INSTALL_PLUGINDIR ${CMAKE_SOURCE_DIR}/mariadb-install)
	endif()

	add_subdirectory(${VENDOR_DIR}/${VENDOR})
endforeach()

foreach(VENDOR ${VENDORS})
	get_all_targets(_targets ${VENDOR_DIR}/${VENDOR})
	list(LENGTH _targets _total_targets)
	message(STATUS ${VENDOR} " has " ${_total_targets} " targets")
	list(JOIN _targets ", " _target_names)
	message(STATUS ${_target_names})
	if(_total_targets GREATER_EQUAL 1)
		foreach(_target IN LISTS _targets)
			get_target_property(_target_type ${_target} TYPE)
			if((NOT _target_type STREQUAL "MODULE_LIBRARY") AND (NOT _target_type STREQUAL "UTILITY") AND (NOT _target STREQUAL "uninstall"))
				message(STATUS "Auto-Linking vendor... " ${_target})
				target_link_libraries(VCMPLua PRIVATE ${_target})
			endif()
		endforeach()
		# list(GET _targets 0 _main_target)
	else()
		message(NOTICE "[!] Skip linking " ${VENDOR} " as it has no targets to link to (header-only libs)")
	endif()
endforeach()

# Vendors that produce more than 1 target have to be manually linked
# link_vendor(cpr::cpr)
# link_vendor(lua::lib)
# link_vendor(spdlog::spdlog)
# link_vendor(SQLiteCpp)