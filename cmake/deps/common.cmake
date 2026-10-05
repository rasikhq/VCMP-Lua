# Helpers for the Lua modules built from pinned release tarballs (plan B2).
# Native libraries come from vcpkg; these small modules are compiled here
# against vcpkg's Lua so they can be registered in package.preload.
include(FetchContent)

if(CMAKE_HOST_WIN32)
    # Git for Windows ships GNU patch.
    find_package(Git QUIET)
    if(GIT_EXECUTABLE)
        get_filename_component(_vcmp_lua_git_dir "${GIT_EXECUTABLE}" DIRECTORY)
        set(_vcmp_lua_patch_hints "${_vcmp_lua_git_dir}/../usr/bin" "${_vcmp_lua_git_dir}/../../usr/bin")
    endif()
endif()
find_program(VCMP_LUA_PATCH_EXECUTABLE patch HINTS ${_vcmp_lua_patch_hints} REQUIRED)

# vcmp_lua_fetch(<name> URL <url> SHA256 <hash> [PATCHES <file>...])
#
# Downloads a tarball, checks its SHA256, unpacks it and applies our patches.
# Sets <name>_SOURCE_DIR in the caller's scope; defines no targets.
macro(vcmp_lua_fetch name)
    cmake_parse_arguments(_vcmp_lua_fetch "" "URL;SHA256" "PATCHES" ${ARGN})
    set(_vcmp_lua_patch_args "")
    foreach(_vcmp_lua_patch IN LISTS _vcmp_lua_fetch_PATCHES)
        if(_vcmp_lua_patch_args)
            list(APPEND _vcmp_lua_patch_args COMMAND)
        else()
            list(APPEND _vcmp_lua_patch_args PATCH_COMMAND)
        endif()
        list(APPEND _vcmp_lua_patch_args
            "${VCMP_LUA_PATCH_EXECUTABLE}" -p1 --forward --batch -i "${_vcmp_lua_patch}")
    endforeach()
    # SOURCE_SUBDIR names a directory without a CMakeLists.txt, so
    # FetchContent_MakeAvailable() only populates the sources.
    FetchContent_Declare(${name}
        URL "${_vcmp_lua_fetch_URL}"
        URL_HASH "SHA256=${_vcmp_lua_fetch_SHA256}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        ${_vcmp_lua_patch_args}
        SOURCE_SUBDIR vcmp-lua-no-cmake-project)
    FetchContent_MakeAvailable(${name})
endmacro()

# vcmp_lua_c_module(<target> SOURCES <file>... [INCLUDE_DIRS <dir>...]
#                   [DEFINITIONS <def>...] [LIBRARIES <lib>...])
#
# A Lua C module compiled into the plugin. The plugin registers its luaopen_*
# function in package.preload (src/runtime/preload.cpp).
function(vcmp_lua_c_module target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;INCLUDE_DIRS;DEFINITIONS;LIBRARIES")
    add_library(${target} STATIC ${arg_SOURCES})
    target_include_directories(${target} PRIVATE ${arg_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE
        ${arg_DEFINITIONS}
        $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
    target_link_libraries(${target} PUBLIC lua ${arg_LIBRARIES})
    vcmp_lua_harden(${target})
endfunction()

# vcmp_lua_embed_lua(<module.name> <file.lua> [...])
#
# Adds pure-Lua modules to the list that src/CMakeLists.txt embeds.
function(vcmp_lua_embed_lua)
    set_property(GLOBAL APPEND PROPERTY VCMP_LUA_EMBEDDED_MODULES ${ARGN})
endfunction()
